//
// Created by robert on 5/19/22.
//

#include <memory.h>
#include "eth.h"
#include "icmp.h"
#include "ip.h"
#include "util.h"
#include "../net.h"

static void sendPingResponse(uint8_t *frame, int flen);

void ICMP_process(uint8_t *frame, int flen) {
    // discard malformed packets
    if(flen < 42) return;
    // map headers
    struct FRAME_ETH *headerEth = (struct FRAME_ETH *) frame;
    struct HEADER_IPv4 *headerIPv4 = (struct HEADER_IPv4 *) (headerEth + 1);
    struct HEADER_ICMPv4 *headerICMP = (struct HEADER_ICMPv4 *) (headerIPv4 + 1);

    // verify destination
    uint32_t dest;
    copyIPv4(&dest, headerIPv4->dst);
    if(dest != ipAddress) return;

    switch (headerICMP->type) {
        // echo request
        case 8:
            if(headerICMP->code == 0)
                sendPingResponse(frame, flen);
            break;
    }
}

static void sendPingResponse(uint8_t *frame, int flen) {
    // map headers
    struct FRAME_ETH *headerEth = (struct FRAME_ETH *) frame;
    struct HEADER_IPv4 *headerIPv4 = (struct HEADER_IPv4 *) (headerEth + 1);
    struct HEADER_ICMPv4 *headerICMP = (struct HEADER_ICMPv4 *) (headerIPv4 + 1);

    // change type to echo response
    headerICMP->type = 0;
    // touch up checksum
    if (headerICMP->chksum[0] > 0xF7)
        ++headerICMP->chksum[1];
    headerICMP->chksum[0] += 0x08;

    // modify IP header
    copyIPv4(headerIPv4->dst, headerIPv4->src);
    copyIPv4(headerIPv4->src, &ipAddress);
    headerIPv4->chksum[0] = 0;
    headerIPv4->chksum[1] = 0;
    RFC1071_checksum(headerIPv4, sizeof(struct HEADER_IPv4), headerIPv4->chksum);

    // modify ethernet frame header
    copyMAC(headerEth->macDst, headerEth->macSrc);

    // get TX descriptor
    int txDesc = NET_getTxDesc();
    if(txDesc < 0) return;
    // transmit response
    memcpy(NET_getTxBuff(txDesc), frame, flen);
    NET_transmit(txDesc, flen);
}
