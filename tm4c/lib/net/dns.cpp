//
// Created by robert on 12/4/22.
//

#include "dns.hpp"

#include "arp.hpp"
#include "eth.hpp"
#include "ip.hpp"
#include "udp.hpp"
#include "util.hpp"
#include "../net.h"
#include "../clk/mono.h"

#include <cstring>

#define DNS_HEAD_SIZE (UDP_DATA_OFFSET + 12)
#define DNS_CLIENT_PORT (1367)
#define DNS_SERVER_PORT (53)
#define MAX_REQUESTS (16)
#define REQUEST_EXPIRE (5)

struct [[gnu::packed]] HEADER_DNS {
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

static_assert(sizeof(HEADER_DNS) == 12, "HEADER_DNS must be 12 bytes");

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
    lastArp = CLK_MONO_INT();
    if (IPv4_testSubnet(ipSubnet, ipAddress, ipDNS))
        copyMAC(dnsMAC, macRouter);
    else
        ARP_request(ipDNS, callbackARP, nullptr);
}

int DNS_lookup(const char *hostname, const CallbackDNS callback, volatile void *ref) {
    // check MAC address age
    const uint32_t now = CLK_MONO_INT();
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
    if (flen < DNS_HEAD_SIZE)
        return;
    // map headers
    const auto headerEth = reinterpret_cast<HeaderEthernet*>(frame);
    const auto headerIP4 = reinterpret_cast<HeaderIp4*>(headerEth + 1);
    const auto headerUDP = reinterpret_cast<HeaderUdp4*>(headerIP4 + 1);
    const auto headerDNS = reinterpret_cast<HEADER_DNS*>(headerUDP + 1);
    // verify destination
    if (isMyMAC(headerEth->macDst))
        return;
    if (headerIP4->dst != ipAddress)
        return;

    // must be a query response
    if (headerDNS->qr != 1)
        return;
    // must be a standard query response
    if (headerDNS->opcode != 0)
        return;

    // find matching request
    int match = -1;
    for (int i = 0; i < MAX_REQUESTS; i++) {
        if (requests[i].callback != nullptr && requests[i].requestId == headerDNS->id) {
            match = i;
            break;
        }
    }
    // discard unknown requests
    if (match < 0)
        return;

    // process response body
    const char *end = reinterpret_cast<char*>(frame + flen);
    char *body = reinterpret_cast<char*>(headerDNS + 1);
    char *next = body;

    // skip over question if present
    uint16_t cnt = htons(headerDNS->qcount);
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
    cnt = htons(headerDNS->ancount);
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
        const uint16_t atype = htons(*(uint16_t*) next);
        next += 2;
        const uint16_t aclass = htons(*(uint16_t*) next);
        next += 2;
        next += 4; // skip TTL field
        const uint16_t length = htons(*(uint16_t*) next);
        next += 2;
        // guard against malformed packets
        if (next >= end)
            return;
        // only accept A record IN IPv4 answers
        if (atype != 1) {
            next += length;
            continue;
        }
        if (aclass != 1) {
            next += length;
            continue;
        }
        if (length != 4) {
            next += length;
            continue;
        }
        // read address
        const uint32_t addr = *(uint32_t*) next;
        next += 4;
        // report address via callback
        (*requests[match].callback)(requests[match].ref, addr);
    }
    // clear request slot
    requests[match].callback = nullptr;
}

static void sendRequest(const char *hostname, const uint16_t requestId) {
    const int txDesc = NET_getTxDesc();
    uint8_t *frame = NET_getTxBuff(txDesc);

    // clear frame buffer
    memset(frame, 0, DNS_HEAD_SIZE);

    // map headers
    const auto headerEth = reinterpret_cast<HeaderEthernet*>(frame);
    const auto headerIP4 = reinterpret_cast<HeaderIp4*>(headerEth + 1);
    const auto headerUDP = reinterpret_cast<HeaderUdp4*>(headerIP4 + 1);
    const auto headerDNS = reinterpret_cast<HEADER_DNS*>(headerUDP + 1);

    // MAC address
    copyMAC(headerEth->macDst, dnsMAC);

    // IPv4 Header
    IPv4_init(frame);
    headerIP4->dst = ipDNS;
    headerIP4->src = ipAddress;
    headerIP4->proto = IP_PROTO_UDP;

    // UDP Header
    headerUDP->portSrc = htons(DNS_CLIENT_PORT);
    headerUDP->portDst = htons(DNS_SERVER_PORT);

    // set header fields
    headerDNS->id = requestId;
    headerDNS->rd = 1;
    headerDNS->qcount = htons(1);

    // append query
    char *base = reinterpret_cast<char*>(headerDNS + 1);
    char *tail = base;
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
    const int flen = DNS_HEAD_SIZE + (tail - base);
    UDP_finalize(frame, flen);
    IPv4_finalize(frame, flen);
    NET_transmit(txDesc, flen);
}
