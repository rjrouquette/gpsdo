//
// Created by robert on 12/4/22.
//

#include "dns.hpp"

#include "arp.hpp"
#include "eth.hpp"
#include "ip.hpp"
#include "udp.hpp"
#include "util.hpp"
#include "../net.hpp"
#include "../clock/mono.hpp"

#include <cstring>

#define DNS_CLIENT_PORT (1367)
#define DNS_SERVER_PORT (53)
#define MAX_REQUESTS (16)
#define REQUEST_EXPIRE (5)

struct [[gnu::packed]] HeaderDns {
    uint16_t id;
    uint16_t rd     : 1;
    uint16_t tc     : 1;
    uint16_t aa     : 1;
    uint16_t opcode : 4;
    uint16_t qr     : 1;
    uint16_t rcode  : 4;
    uint16_t zero   : 3;
    uint16_t ra     : 1;
    uint16_t qcount;
    uint16_t ancount;
    uint16_t nscount;
    uint16_t arcount;
};

static_assert(sizeof(HeaderDns) == 12, "HeaderDns must be 12 bytes");

struct [[gnu::packed]] FrameDns : FrameUdp4 {
    static constexpr int DATA_OFFSET = FrameUdp4::DATA_OFFSET + sizeof(HeaderDns);

    HeaderDns dns;
    char body[0];

    static auto& from(void *frame) {
        return *static_cast<FrameDns*>(frame);
    }

    static auto& from(const void *frame) {
        return *static_cast<const FrameDns*>(frame);
    }
};

static_assert(sizeof(FrameDns) == 54, "FrameDns must be 54 bytes");

static struct {
    uint32_t expire;
    uint32_t requestId;
    CallbackDNS callback;
    void *ref;
} requests[MAX_REQUESTS];

static uint32_t lastArp;
static uint16_t nextRequest;
static uint8_t dnsMAC[6];

static void processFrame(uint8_t *frame, int flen);
static void sendRequest(const char *hostname, uint16_t requestId);

void DNS_init() {
    // initialize request ID to lower 16-bit of MAC address
    uint8_t mac[6];
    getMAC(mac);
    nextRequest = mac[1];
    nextRequest <<= 8;
    nextRequest |= mac[0];

    // register DNS client port
    UDP_register(DNS_CLIENT_PORT, processFrame);
}

static void callbackARP(void *ref, const uint32_t remoteAddress, const uint8_t *macAddress) {
    if (remoteAddress == ipDNS) {
        // retry if request timed-out
        if (isNullMAC(macAddress))
            DNS_updateMAC();
        else
            copyMAC(dnsMAC, macAddress);
    }
}

void DNS_updateMAC() {
    lastArp = clock::monotonic::seconds();
    if (IPv4_testSubnet(ipSubnet, ipAddress, ipDNS))
        copyMAC(dnsMAC, macRouter);
    else
        ARP_request(ipDNS, callbackARP, nullptr);
}

int DNS_lookup(const char *hostname, const CallbackDNS callback, volatile void *ref) {
    // check MAC address age
    const uint32_t now = clock::monotonic::seconds();
    if ((now - lastArp) > ARP_MAX_AGE)
        DNS_updateMAC();

    // abort if MAC address is invalid
    if (isNullMAC(dnsMAC))
        return -2;

    for (auto &request : requests) {
        // look for empty or expired slot
        if (request.callback != nullptr && static_cast<int32_t>(now - request.expire) > 0)
            continue;

        // register callback
        request.callback = callback;
        request.ref = const_cast<void*>(ref);
        request.requestId = nextRequest++;
        request.expire = now + REQUEST_EXPIRE;

        // send request
        sendRequest(hostname, request.requestId);
        return 0;
    }
    return -1;
}

static void processFrame(uint8_t *frame, const int flen) {
    // discard malformed packets
    if (flen < FrameDns::DATA_OFFSET)
        return;
    // map headers
    const auto &packet = FrameDns::from(frame);
    // verify destination
    if (isMyMAC(packet.eth.macDst))
        return;
    if (packet.ip4.dst != ipAddress)
        return;

    // must be a query response
    if (packet.dns.qr != 1)
        return;
    // must be a standard query response
    if (packet.dns.opcode != 0)
        return;

    // find matching request
    int match = -1;
    for (int i = 0; i < MAX_REQUESTS; i++) {
        if (requests[i].callback != nullptr && requests[i].requestId == packet.dns.id) {
            match = i;
            break;
        }
    }
    // discard unknown requests
    if (match < 0)
        return;

    // process response body
    auto &request = requests[match];
    const auto end = reinterpret_cast<const char*>(frame + flen);
    const auto body = packet.body;
    auto next = body;

    // skip over question if present
    uint16_t cnt = htons(packet.dns.qcount);
    for (uint16_t i = 0; i < cnt; i++) {
        // skip over name
        while (next < end) {
            const uint8_t len = *next++;
            if (len == 0)
                break;
            if (len == 0xC0) {
                ++next;
                break;
            }
            next += len;
        }
        // skip over type and class
        next += 4;
    }
    // guard against malformed packets
    if (next >= end)
        return;

    // process answers
    cnt = htons(packet.dns.ancount);
    for (uint16_t i = 0; i < cnt; i++) {
        // skip over name
        while (next < end) {
            const uint8_t len = *(next++);
            if (len == 0)
                break;
            if (len == 0xC0) {
                ++next;
                break;
            }
            next += len;
        }
        // guard against malformed packets
        if (next >= end)
            return;
        const auto atype = htons(*reinterpret_cast<const uint16_t*>(next));
        next += 2;
        const auto aclass = htons(*reinterpret_cast<const uint16_t*>(next));
        next += 2;
        next += 4; // skip TTL field
        const auto length = htons(*reinterpret_cast<const uint16_t*>(next));
        next += 2;
        // guard against malformed packets
        if (next >= end)
            return;
        // only accept A record IN IPv4 answers
        if (atype != 1 || aclass != 1 || length != 4) {
            next += length;
            continue;
        }
        // read address
        const auto addr = *reinterpret_cast<const uint32_t*>(next);
        next += 4;
        // report address via callback
        (*request.callback)(request.ref, addr);
    }
    // clear request slot
    request.callback = nullptr;
}

static void sendRequest(const char *hostname, const uint16_t requestId) {
    const int txDesc = NET_getTxDesc();
    uint8_t *frame = NET_getTxBuff(txDesc);

    // clear frame buffer
    memset(frame, 0, FrameDns::DATA_OFFSET);

    // map headers
    auto &packet = FrameDns::from(frame);

    // MAC address
    copyMAC(packet.eth.macDst, dnsMAC);

    // IPv4 Header
    IPv4_init(frame);
    packet.ip4.dst = ipDNS;
    packet.ip4.src = ipAddress;
    packet.ip4.proto = IP_PROTO_UDP;

    // UDP Header
    packet.udp.portSrc = htons(DNS_CLIENT_PORT);
    packet.udp.portDst = htons(DNS_SERVER_PORT);

    // set header fields
    packet.dns.id = requestId;
    packet.dns.rd = 1;
    packet.dns.qcount = htons(1);

    // append query
    const auto base = packet.body;
    auto tail = base;
    // append domain name
    for (;;) {
        // skip dots
        if (*hostname == '.')
            ++hostname;
        // check for null-terminator
        if (*hostname == 0)
            break;

        // pin section length byte
        char *lenbyte = tail++;
        // append section characters
        while (*hostname != 0 && *hostname != '.')
            *(tail++) = *(hostname++);
        // set section length byte
        *lenbyte = (tail - lenbyte) - 1;
    }
    *tail++ = 0;
    *reinterpret_cast<uint16_t*>(tail) = htons(1);
    tail += 2;
    *reinterpret_cast<uint16_t*>(tail) = htons(1);
    tail += 2;

    // transmit request
    const int flen = FrameDns::DATA_OFFSET + (tail - base);
    UDP_finalize(frame, flen);
    IPv4_finalize(frame, flen);
    NET_transmit(txDesc, flen);
}
