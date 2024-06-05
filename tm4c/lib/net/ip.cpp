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

static constexpr struct {
    uint8_t proto;
    void (*handler)(uint8_t *frame, int size);
} registry[] = {
    {IP_PROTO_ICMP, ICMP_process},
    {IP_PROTO_UDP, UDP_process}
};

void IPv4_process(uint8_t *frame, const int size) {
    // discard malformed frames
    if (size < 60)
        return;

    const auto &packet = FrameIp4::from(frame);
    // must be version 4
    if (packet.ip4.head.VER != 4)
        return;
    // must have standard 5 word header
    if (packet.ip4.head.IHL != 5)
        return;

    for (const auto &entry : registry) {
        if(packet.ip4.proto == entry.proto) {
            (*entry.handler)(frame, size);
            break;
        }
    }
}

volatile uint16_t ipID = 0x1234;

void IPv4_init(uint8_t *frame) {
    auto &packet = FrameIp4::from(frame);
    packet.ip4.head.VER = 4;
    packet.ip4.head.IHL = 5;
    packet.ip4.id = htons(ipID);
    packet.ip4.flags = 0x40;
    packet.ip4.ttl = 32;
    ++ipID;
}

void IPv4_finalize(uint8_t *frame, int flen) {
    // map headers
    auto &packet = FrameIp4::from(frame);

    // set EtherType
    packet.eth.ethType = ETHTYPE_IP4;
    // compute IPv4 length
    flen -= sizeof(HeaderEthernet);
    // set IPv4 length
    packet.ip4.len = htons(flen);
    // clear checksum
    packet.ip4.chksum = 0;
    // compute checksum
    packet.ip4.chksum = RFC1071_checksum(&packet.ip4, sizeof(HeaderIp4));
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
    auto &packet = FrameIp4::from(frame);

    // set MAC address
    IPv4_macMulticast(packet.eth.macDst, groupAddress);
    // set group address
    packet.ip4.dst = groupAddress;
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
