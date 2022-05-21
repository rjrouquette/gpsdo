//
// Created by robert on 5/20/22.
//

#include "eth.h"
#include "ip.h"
#include "tcp.h"
#include "util.h"


#define MAX_ENTRIES (8)
static volatile uint16_t registryPort[MAX_ENTRIES];
static volatile CallbackTCP registryCallback[MAX_ENTRIES];


void TCP_process(uint8_t *frame, int flen) {
    // discard malformed frames
    if(flen < 62) return;
    
    // map headers
    struct FRAME_ETH *headerEth = (struct FRAME_ETH *) frame;
    struct HEADER_IPv4 *headerIPv4 = (struct HEADER_IPv4 *) (headerEth + 1);
    struct HEADER_TCP *headerTCP = (struct HEADER_TCP *) (headerIPv4 + 1);

    // load port number
    uint16_t port;
    ((uint8_t *) &port)[1] = headerTCP->portDst[0];
    ((uint8_t *) &port)[0] = headerTCP->portDst[1];
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

void TCP_finalize(uint8_t *frame, int flen) {
    struct HEADER_IPv4 *headerIPv4 = (struct HEADER_IPv4 *) (frame + sizeof(struct FRAME_ETH));
    struct HEADER_TCP *headerTCP = (struct HEADER_TCP *) (headerIPv4 + 1);

    // compute TCP length
    flen -= sizeof(struct FRAME_ETH);
    flen -= sizeof(struct HEADER_IPv4);
    // clear checksum field
    headerTCP->chksum[0] = 0;
    headerTCP->chksum[1] = 0;

    // partial checksum of header and data
    RFC1071_checksum(headerTCP, flen, headerTCP->chksum);

    // append pseudo header to checksum
    uint8_t chkbuf[] = {
            // partial checksum
            ~headerTCP->chksum[0], ~headerTCP->chksum[1],
            // source address
            headerIPv4->src[0], headerIPv4->src[1], headerIPv4->src[2], headerIPv4->src[3],
            // destination address
            headerIPv4->dst[0], headerIPv4->dst[1], headerIPv4->dst[2], headerIPv4->dst[3],
            // protocol
            0, headerIPv4->proto,
            // TCP length
            (flen >> 8) & 0xFF, (flen >> 0) & 0xFF
    };
    // validate buffer size
    _Static_assert(sizeof(chkbuf) == 14, "pseudo header checksum buffer must be 14 bytes");

    // finalize checksum calculation
    RFC1071_checksum(chkbuf, sizeof(chkbuf), headerTCP->chksum);
}

int TCP_register(uint16_t port, CallbackTCP callback) {
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

int TCP_deregister(uint16_t port) {
    for(int i = 0; i < MAX_ENTRIES; i++) {
        if(registryPort[i] == port) {
            registryPort[i] = 0;
            registryCallback[i] = 0;
            return 0;
        }
    }
    return -1;
}

int TCP_delegate(uint8_t *frame, int flen, struct TCP_CONN *tcpConn, int cntConn) {
    // map headers
    struct FRAME_ETH *headerEth = (struct FRAME_ETH *) frame;
    struct HEADER_IPv4 *headerIPv4 = (struct HEADER_IPv4 *) (headerEth + 1);
    struct HEADER_TCP *headerTCP = (struct HEADER_TCP *) (headerIPv4 + 1);

    // load destination
    uint32_t dstIP;
    copyIPv4(&dstIP, headerIPv4->dst);
    uint16_t dstPort;
    ((uint8_t *) &dstPort)[1] = headerTCP->portDst[0];
    ((uint8_t *) &dstPort)[0] = headerTCP->portDst[1];
    // load source
    uint32_t srcIP;
    copyIPv4(&srcIP, headerIPv4->src);
    uint16_t srcPort;
    ((uint8_t *) &srcPort)[1] = headerTCP->portSrc[0];
    ((uint8_t *) &srcPort)[0] = headerTCP->portSrc[1];

    int target = -1;
    for(int i = 0; i < cntConn; i++) {
        if(tcpConn->state.active) {
            // check for matching connection
            if(
                    (tcpConn->addrLocal.ip4 == dstIP && tcpConn->addrLocal.port == dstPort) &&
                    (tcpConn->addrRemote.ip4 == srcIP && tcpConn->addrRemote.port == srcPort)
            ) {
                target = i;
                break;
            }
        }
        else if(target < 0) {
            // track first available
            target = i;
        }
    }

    return -target;
}
