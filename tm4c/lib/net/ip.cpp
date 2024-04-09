//
// Created by robert on 5/16/22.
//

#include "ip.hpp"

#include "eth.hpp"
#include "icmp.hpp"
#include "udp.hpp"
#include "util.hpp"

volatile uint32_t ipBroadcast = 0;
volatile uint32_t ipAddress = 0;
volatile uint32_t ipSubnet = -1;
volatile uint32_t ipRouter = 0;
volatile uint32_t ipDNS = 0;

void IPv4_process(uint8_t *frame, const int flen) {
    // discard malformed frames
    if (flen < 60)
        return;

    const auto headerIPv4 = reinterpret_cast<struct HeaderIp4*>(frame + sizeof(struct HeaderEthernet));
    // must be version 4
    if (headerIPv4->head.VER != 4)
        return;
    // must standard 5 word header
    if (headerIPv4->head.IHL != 5)
        return;

    // process ICMP frame
    if (headerIPv4->proto == IP_PROTO_ICMP) {
        ICMP_process(frame, flen);
        return;
    }
    // process UDP frame
    if (headerIPv4->proto == IP_PROTO_UDP) {
        UDP_process(frame, flen);
        return;
    }
}

volatile uint16_t ipID = 0x1234;

void IPv4_init(uint8_t *frame) {
    const auto headerIPv4 = reinterpret_cast<struct HeaderIp4*>(frame + sizeof(struct HeaderEthernet));
    headerIPv4->head.VER = 4;
    headerIPv4->head.IHL = 5;
    headerIPv4->id = htons(ipID);
    headerIPv4->flags = 0x40;
    headerIPv4->ttl = 32;
    ++ipID;
}

void IPv4_finalize(uint8_t *frame, int flen) {
    // map headers
    const auto headerEth = reinterpret_cast<HeaderEthernet*>(frame);
    const auto headerIP4 = reinterpret_cast<HeaderIp4*>(headerEth + 1);

    // set EtherType
    headerEth->ethType = ETHTYPE_IP4;
    // compute IPv4 length
    flen -= sizeof(HeaderEthernet);
    // set IPv4 length
    headerIP4->len = htons(flen);
    // clear checksum
    headerIP4->chksum = 0;
    // compute checksum
    headerIP4->chksum = RFC1071_checksum(headerIP4, sizeof(HeaderIp4));
}

void IPv4_macMulticast(uint8_t *mac, const uint32_t groupAddress) {
    mac[0] = 0x01;
    mac[1] = 0x00;
    mac[2] = 0x5E;
    mac[3] = (groupAddress >> 8) & 0x7F;
    mac[4] = (groupAddress >> 16) & 0xFF;
    mac[5] = (groupAddress >> 24) & 0xFF;
}

void IPv4_setMulticast(uint8_t *frame, const uint32_t groupAddress) {
    // map headers
    const auto headerEth = reinterpret_cast<HeaderEthernet*>(frame);
    const auto headerIP4 = reinterpret_cast<HeaderIp4*>(headerEth + 1);

    // set MAC address
    IPv4_macMulticast(headerEth->macDst, groupAddress);
    // set group address
    headerIP4->dst = groupAddress;
}

uint16_t RFC1071_checksum(volatile const void *buffer, const int len) {
    auto ptr = static_cast<const uint16_t*>(const_cast<const void*>(buffer));
    const auto end = ptr + (len >> 1);
    uint32_t sum = 0;
    while (ptr < end) {
        sum += *ptr++;
    }
    if (len & 1)
        sum += *reinterpret_cast<const uint8_t*>(ptr);
    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);
    return ~sum;
}
