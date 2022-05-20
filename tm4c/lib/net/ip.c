//
// Created by robert on 5/16/22.
//

#include <memory.h>

#include "../format.h"
#include "eth.h"
#include "icmp.h"
#include "ip.h"
#include "tcp.h"
#include "udp.h"
#include "util.h"

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


extern volatile uint8_t debugMac[6];

void IPv4_process(uint8_t *frame, int flen) {
    // discard malformed frames
    if(flen < 64) return;

    struct HEADER_IPv4 *headerIPv4 = (struct HEADER_IPv4 *) (frame + sizeof(struct FRAME_ETH));
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
        return;
    }
    // process TCP frame
    if(headerIPv4->proto == IP_PROTO_TCP) {
        TCP_process(frame, flen);
        return;
    }
    // process UDP frame
    if(headerIPv4->proto == IP_PROTO_UDP) {
        UDP_process(frame, flen);
        return;
    }
}

void RFC1071_checksum(volatile const void *buffer, int len, volatile void *result) {
    uint16_t *ptr = (uint16_t *) buffer;
    uint16_t *end = ptr + (len >> 1);
    uint32_t sum = 0;
    while(ptr < end) {
        sum += *(ptr++);
    }
    if(len & 1)
        sum += *(uint8_t *)ptr;
    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);
    // store result
    *((uint16_t *)result) = ~sum;
}
