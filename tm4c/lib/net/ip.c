//
// Created by robert on 5/16/22.
//

#include <memory.h>

#include "arp.h"
#include "eth.h"
#include "ip.h"
#include "../format.h"
#include "util.h"
#include "icmp.h"

volatile uint32_t ipAddress = 0;
volatile uint32_t ipSubnet = -1;
volatile uint32_t ipGateway = 0;
volatile uint32_t ipDNS = 0;


void NET_getIpAddress(char *strAddr) {
    strAddr += toDec((ipAddress >> 0u) & 0xFFu, 0, '0', strAddr);
    *(strAddr++) = '.';
    strAddr += toDec((ipAddress >> 8u) & 0xFFu, 0, '0', strAddr);
    *(strAddr++) = '.';
    strAddr += toDec((ipAddress >> 16u) & 0xFFu, 0, '0', strAddr);
    *(strAddr++) = '.';
    strAddr += toDec((ipAddress >> 24u) & 0xFFu, 0, '0', strAddr);
    *strAddr = 0;
}


void IPv4_process(uint8_t *frame, int flen) {
    // discard malformed packets
    if(flen < 64) return;

    struct HEADER_IPv4 *headerIPv4 = (struct HEADER_IPv4 *) (frame + sizeof(struct HEADER_ETH));
    // must be version 4
    if(headerIPv4->head.VER != 4) return;
    // must standard 5 word header
    if(headerIPv4->head.IHL != 5) return;
    // verify destination
    uint32_t dest;
    copyIPv4(&dest, headerIPv4->dest);
    if(dest != ipAddress) return;

    // process ICMP frame
    if(headerIPv4->proto == IP_PROTO_ICMP) {
        ICMP_process(frame, flen);
    }
}
