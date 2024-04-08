//
// Created by robert on 5/20/22.
//

#include "eth.hpp"
#include "ip.hpp"
#include "udp.hpp"
#include "util.hpp"

#define MAX_ENTRIES (16)
static volatile uint16_t registryPort[MAX_ENTRIES];
static volatile CallbackUDP registryCallback[MAX_ENTRIES];


void UDP_process(uint8_t *frame, int flen) {
    // map headers
    struct HEADER_ETH *headerEth = (struct HEADER_ETH *) frame;
    struct HEADER_IP4 *headerIPv4 = (struct HEADER_IP4 *) (headerEth + 1);
    struct HEADER_UDP *headerUDP = (struct HEADER_UDP *) (headerIPv4 + 1);

    // load port number
    uint16_t port = htons(headerUDP->portDst);
    // discard if port is invalid
    if(port == 0) return;

    // invoke port handler
    for(int i = 0; i < MAX_ENTRIES; i++) {
        if(registryPort[i] == port) {
            (*(registryCallback[i]))(frame, flen);
            break;
        }
    }
}

void UDP_finalize(uint8_t *frame, int flen) {
    HEADER_IP4 *headerIPv4 = (struct HEADER_IP4 *) (frame + sizeof(struct HEADER_ETH));
    HEADER_UDP *headerUDP = (struct HEADER_UDP *) (headerIPv4 + 1);

    // compute UDP length
    flen -= sizeof(struct HEADER_ETH);
    flen -= sizeof(struct HEADER_IP4);
    // set UDP length
    headerUDP->length = htons(flen);
    // clear checksum field
    headerUDP->chksum = 0;
    // partial checksum of header and data
    uint16_t partial = RFC1071_checksum(headerUDP, flen);

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
            .addrSrc = headerIPv4->src,
            // destination address
            .addrDst = headerIPv4->dst,
            // udp length
            .length = headerUDP->length,
            // partial checksum
            .chksum = static_cast<uint16_t>(~partial),
            // protocol
            .zero = 0, .proto = headerIPv4->proto
    };
    // validate buffer size
    static_assert(sizeof(chkbuf) == 14, "pseudo header checksum buffer must be 14 bytes");

    // finalize checksum calculation
    headerUDP->chksum = RFC1071_checksum(&chkbuf, sizeof(chkbuf));
}

void UDP_returnToSender(uint8_t *frame, uint32_t ipAddr, uint16_t port) {
    // map headers
    struct HEADER_ETH *headerEth = (struct HEADER_ETH *) frame;
    struct HEADER_IP4 *headerIPv4 = (struct HEADER_IP4 *) (headerEth + 1);
    struct HEADER_UDP *headerUDP = (struct HEADER_UDP *) (headerIPv4 + 1);

    // modify ethernet frame header
    copyMAC(headerEth->macDst, headerEth->macSrc);
    // modify IP header
    headerIPv4->dst = headerIPv4->src;
    headerIPv4->src = ipAddr;
    // modify UDP header
    headerUDP->portDst = headerUDP->portSrc;
    headerUDP->portSrc = htons(port);
}

int UDP_register(uint16_t port, CallbackUDP callback) {
    for(int i = 0; i < MAX_ENTRIES; i++) {
        if(registryPort[i] == port)
            return (registryCallback[i] == callback) ? 0 : -1;
    }
    for(int i = 0; i < MAX_ENTRIES; i++) {
        if(registryPort[i] == 0) {
            registryPort[i] = port;
            registryCallback[i] = callback;
            return 0;
        }
    }
    return -2;
}

int UDP_deregister(uint16_t port) {
    for(int i = 0; i < MAX_ENTRIES; i++) {
        if(registryPort[i] == port) {
            registryPort[i] = 0;
            registryCallback[i] = 0;
            return 0;
        }
    }
    return -1;
}
