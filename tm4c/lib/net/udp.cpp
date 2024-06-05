//
// Created by robert on 5/20/22.
//

#include "eth.hpp"
#include "ip.hpp"
#include "udp.hpp"
#include "util.hpp"


static volatile struct {
    uint16_t port;
    CallbackUDP callback;
} registry[16];


void FrameUdp4::returnToSender() {
    const auto ipAddr = ip4.dst;
    const auto port = udp.portDst;

    // modify ethernet frame header
    copyMAC(eth.macDst, eth.macSrc);
    // modify IP header
    ip4.dst = ip4.src;
    ip4.src = ipAddr;
    // modify UDP header
    udp.portDst = udp.portSrc;
    udp.portSrc = port;
}

void FrameUdp4::returnToSender(const uint32_t ipAddr, const uint16_t port) {
    // modify ethernet frame header
    copyMAC(eth.macDst, eth.macSrc);
    // modify IP header
    ip4.dst = ip4.src;
    ip4.src = ipAddr;
    // modify UDP header
    udp.portDst = udp.portSrc;
    udp.portSrc = htons(port);
}

void UDP_process(uint8_t *frame, const int size) {
    // load port number
    const uint16_t port = htons(FrameUdp4::from(frame).udp.portDst);
    // discard if port is invalid
    if (port == 0)
        return;

    // invoke port handler
    for (const auto &entry : registry) {
        if (entry.port == port) {
            (*entry.callback)(frame, size);
            break;
        }
    }
}

void UDP_finalize(uint8_t *frame, int size) {
    auto &packet = FrameUdp4::from(frame);

    // compute UDP length
    size -= sizeof(HeaderEthernet);
    size -= sizeof(HeaderIp4);
    // set UDP length
    packet.udp.length = htons(size);
    // clear checksum field
    packet.udp.chksum = 0;
    // partial checksum of header and data
    const uint16_t partial = RFC1071_checksum(&packet.udp, size);

    // append pseudo header to checksum
    const struct [[gnu::packed]] {
        uint32_t addrSrc;
        uint32_t addrDst;
        uint16_t length;
        uint16_t chksum;
        uint8_t zero;
        uint8_t proto;
    } chkbuf = {
            // source address
            .addrSrc = packet.ip4.src,
            // destination address
            .addrDst = packet.ip4.dst,
            // udp length
            .length = packet.udp.length,
            // partial checksum
            .chksum = static_cast<uint16_t>(~partial),
            // protocol
            .zero = 0, .proto = packet.ip4.proto
        };
    // validate buffer size
    static_assert(sizeof(chkbuf) == 14, "pseudo header checksum buffer must be 14 bytes");

    // finalize checksum calculation
    packet.udp.chksum = RFC1071_checksum(&chkbuf, sizeof(chkbuf));
}

int UDP_register(const uint16_t port, const CallbackUDP callback) {
    for (const auto &entry : registry) {
        if (entry.port == port)
            return (entry.callback == callback) ? 0 : -1;
    }
    for (auto &entry : registry) {
        if (entry.port == 0) {
            entry.port = port;
            entry.callback = callback;
            return 0;
        }
    }
    return -2;
}

int UDP_deregister(const uint16_t port) {
    for (auto &entry : registry) {
        if (entry.port == port) {
            entry.port = 0;
            entry.callback = nullptr;
            return 0;
        }
    }
    return -1;
}
