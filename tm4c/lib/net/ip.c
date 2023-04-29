//
// Created by robert on 5/16/22.
//

#include "eth.h"
#include "icmp.h"
#include "ip.h"
#include "udp.h"

volatile uint32_t ipBroadcast = 0;
volatile uint32_t ipAddress = 0;
volatile uint32_t ipSubnet = -1;
volatile uint32_t ipRouter = 0;
volatile uint32_t ipDNS = 0;

void IPv4_process(uint8_t *frame, int flen) {
    // discard malformed frames
    if(flen < 60) return;

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
    headerIPv4->id = __builtin_bswap16(ipID);
    headerIPv4->flags = 0x40;
    headerIPv4->ttl = 32;
    ++ipID;
}

void IPv4_finalize(uint8_t *frame, int flen) {
    struct HEADER_IPv4 *headerIPv4 = (struct HEADER_IPv4 *) (frame + sizeof(struct FRAME_ETH));
    // compute IPv4 length
    flen -= sizeof(struct FRAME_ETH);
    // set IPv4 length
    headerIPv4->len = __builtin_bswap16(flen);
    // clear checksum
    headerIPv4->chksum = 0;
    // compute checksum
    headerIPv4->chksum = RFC1071_checksum(headerIPv4, sizeof(struct HEADER_IPv4));
}

void IPv4_setMulticast(uint8_t *frame, uint32_t groupAddress) {
    // set MAC address
    struct FRAME_ETH *headerEth = (struct FRAME_ETH *) frame;
    headerEth->macDst[0] = 0x01;
    headerEth->macDst[1] = 0x00;
    headerEth->macDst[2] = 0x5E;
    headerEth->macDst[3] = (groupAddress >> 16) & 0x7F;
    headerEth->macDst[4] = (groupAddress >> 8) & 0xFF;
    headerEth->macDst[5] = (groupAddress >> 0) & 0xFF;
    // set group address
    struct HEADER_IPv4 *headerIPv4 = (struct HEADER_IPv4 *) (headerEth + 1);
    headerIPv4->dst = groupAddress;
}

uint16_t RFC1071_checksum(volatile const void *buffer, int len) {
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
    return ~sum;
}
