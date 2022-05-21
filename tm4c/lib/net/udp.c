//
// Created by robert on 5/20/22.
//

#include <memory.h>

#include "eth.h"
#include "ip.h"
#include "udp.h"

#define MAX_ENTRIES (8)
static volatile uint16_t registryPort[MAX_ENTRIES];
static volatile CallbackUDP registryCallback[MAX_ENTRIES];


void UDP_process(uint8_t *frame, int flen) {
    // discard malformed packets
    if(flen < 42) return;
    // map headers
    struct FRAME_ETH *headerEth = (struct FRAME_ETH *) frame;
    struct HEADER_IPv4 *headerIPv4 = (struct HEADER_IPv4 *) (headerEth + 1);
    struct HEADER_UDP *headerUDP = (struct HEADER_UDP *) (headerIPv4 + 1);

    // load port number
    uint16_t port;
    ((uint8_t *) &port)[1] = headerUDP->portDst[0];
    ((uint8_t *) &port)[0] = headerUDP->portDst[1];
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
    struct HEADER_IPv4 *headerIPv4 = (struct HEADER_IPv4 *) (frame + sizeof(struct FRAME_ETH));
    struct HEADER_UDP *headerUDP = (struct HEADER_UDP *) (headerIPv4 + 1);

    // compute UDP length
    flen -= sizeof(struct FRAME_ETH);
    flen -= sizeof(struct HEADER_IPv4);
    // set UDP length
    headerUDP->length[0] = (flen >> 8) & 0xFF;
    headerUDP->length[1] = (flen >> 0) & 0xFF;
    // clear checksum field
    headerUDP->chksum[0] = 0;
    headerUDP->chksum[1] = 0;

    // partial checksum of header and data
    RFC1071_checksum(headerUDP, flen, headerUDP->chksum);

    // append pseudo header to checksum
    uint8_t chkbuf[] = {
            // partial checksum
            ~headerUDP->chksum[0], ~headerUDP->chksum[1],
            // source address
            headerIPv4->src[0], headerIPv4->src[1], headerIPv4->src[2], headerIPv4->src[3],
            // destination address
            headerIPv4->dst[0], headerIPv4->dst[1], headerIPv4->dst[2], headerIPv4->dst[3],
            // protocol
            0, headerIPv4->proto,
            // udp length
            headerUDP->length[0], headerUDP->length[1]
    };
    // validate buffer size
    _Static_assert(sizeof(chkbuf) == 14, "pseudo header checksum buffer must be 14 bytes");

    // finalize checksum calculation
    RFC1071_checksum(chkbuf, sizeof(chkbuf), headerUDP->chksum);
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
