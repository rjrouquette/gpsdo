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

volatile uint16_t ipID = 0x1234;
void IPv4_init(uint8_t *frame) {
    struct HEADER_IPv4 *headerIPv4 = (struct HEADER_IPv4 *) (frame + sizeof(struct FRAME_ETH));
    headerIPv4->head.VER = 4;
    headerIPv4->head.IHL = 5;
    headerIPv4->id[0] = (ipID >> 8) & 0xFF;
    headerIPv4->id[1] = (ipID >> 0) & 0xFF;
    headerIPv4->flags = 0x40;
    headerIPv4->ttl = 32;
    ++ipID;
}

void IPv4_finalize(uint8_t *frame, int flen) {
    struct HEADER_IPv4 *headerIPv4 = (struct HEADER_IPv4 *) (frame + sizeof(struct FRAME_ETH));
    // compute IPv4 length
    flen -= sizeof(struct FRAME_ETH);
    // set IPv4 length
    headerIPv4->len[0] = (flen >> 8) & 0xFF;
    headerIPv4->len[1] = (flen >> 0) & 0xFF;
    // clear checksum
    headerIPv4->chksum[0] = 0;
    headerIPv4->chksum[1] = 0;
    // compute checksum
    RFC1071_checksum(headerIPv4, sizeof(struct HEADER_IPv4), headerIPv4->chksum);
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
