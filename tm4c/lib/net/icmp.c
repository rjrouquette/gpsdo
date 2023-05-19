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
    // map headers
    HEADER_ETH *headerEth = (HEADER_ETH *) frame;
    HEADER_IP4 *headerIP4 = (HEADER_IP4 *) (headerEth + 1);
    HEADER_ICMP4 *headerICMP = (HEADER_ICMP4 *) (headerIP4 + 1);

    // verify destination
    if(headerIP4->dst != ipAddress) return;

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
    HEADER_ETH *headerEth = (HEADER_ETH *) frame;
    HEADER_IP4 *headerIP4 = (HEADER_IP4 *) (headerEth + 1);
    HEADER_ICMP4 *headerICMP = (HEADER_ICMP4 *) (headerIP4 + 1);

    // change type to echo response
    headerICMP->type = 0;
    // touch up checksum
    if (headerICMP->chksum[0] > 0xF7)
        ++headerICMP->chksum[1];
    headerICMP->chksum[0] += 0x08;

    // modify IP header
    headerIP4->dst = headerIP4->src;
    headerIP4->src = ipAddress;
    IPv4_finalize(frame, flen);

    // modify ethernet frame header
    copyMAC(headerEth->macDst, headerEth->macSrc);

    // get TX descriptor
    int txDesc = NET_getTxDesc();
    if(txDesc < 0) return;
    // transmit response
    memcpy(NET_getTxBuff(txDesc), frame, flen);
    NET_transmit(txDesc, flen);
}
