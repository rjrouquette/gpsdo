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
    uint16_t port = __builtin_bswap16(headerTCP->portDst);
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
    headerTCP->chksum = 0;
    // partial checksum of header and data
    uint16_t partial = RFC1071_checksum(headerTCP, flen);

    // append pseudo header to checksum
    struct PACKED {
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
            .length = __builtin_bswap16(flen),
            // partial checksum
            .chksum = ~partial,
            // protocol
            .zero = 0, .proto = headerIPv4->proto
    };
    // validate buffer size
    _Static_assert(sizeof(chkbuf) == 14, "pseudo header checksum buffer must be 14 bytes");

    // finalize checksum calculation
    headerTCP->chksum = RFC1071_checksum(&chkbuf, sizeof(chkbuf));
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


void TCP_CONN_recv(struct TCP_CONN *tcpConn, int cid, void *frame, int flen);

void TCP_delegate(uint8_t *frame, int flen, struct TCP_CONN *tcpConn, int cntConn) {
    // map headers
    struct FRAME_ETH *headerEth = (struct FRAME_ETH *) frame;
    struct HEADER_IPv4 *headerIPv4 = (struct HEADER_IPv4 *) (headerEth + 1);
    struct HEADER_TCP *headerTCP = (struct HEADER_TCP *) (headerIPv4 + 1);

    // load destination
    uint32_t dstIP = headerIPv4->dst;
    uint16_t dstPort = __builtin_bswap16(headerTCP->portDst);
    // load source
    uint32_t srcIP = headerIPv4->src;
    uint16_t srcPort = __builtin_bswap16(headerTCP->portSrc);

    // search for connection instance
    int cid = -1;
    for(int i = 0; i < cntConn; i++) {
        if(tcpConn[i].state.inuse) {
            // check for matching connection
            if(
                    (tcpConn[i].addrLocal.ip4 == dstIP && tcpConn[i].addrLocal.port == dstPort) &&
                    (tcpConn[i].addrRemote.ip4 == srcIP && tcpConn[i].addrRemote.port == srcPort)
            ) {
                cid = i;
                break;
            }
        }
        else if(headerTCP->flags.SYN && cid < 0) {
            // track first available
            cid = i;
        }
    }
    if(cid < 0) {
        // no descriptors available
        if(headerTCP->flags.SYN) {
            // reset connection

        }
        return;
    }
    TCP_CONN_recv(tcpConn + cid, cid, frame, flen);
}

void TCP_CONN_recv(struct TCP_CONN *tcpConn, int cid, void *frame, int flen) {

}

int TCP_CONN_send(struct TCP_CONN *tcpConn, void *data, int len) {
    return -1;
}
