//
// Created by robert on 5/19/22.
//

#include "icmp.hpp"

#include "eth.hpp"
#include "ip.hpp"
#include "util.hpp"
#include "../net.hpp"

#include <memory.h>

struct [[gnu::packed]] HeaderIcmp4 {
    uint8_t type;
    uint8_t code;
    uint8_t chksum[2];
    uint8_t misc[4];
};

static_assert(sizeof(HeaderIcmp4) == 8, "HEADER_ICMP4 must be 8 bytes");


struct [[gnu::packed]] FrameIcmp4 : FrameIp4 {
    static constexpr int DATA_OFFSET = FrameIp4::DATA_OFFSET + sizeof(HeaderIcmp4);

    HeaderIcmp4 icmp;
    uint8_t body[0];

    static auto& from(void *frame) {
        return *static_cast<FrameIcmp4*>(frame);
    }

    static auto& from(const void *frame) {
        return *static_cast<const FrameIcmp4*>(frame);
    }
};

static_assert(sizeof(FrameIcmp4) == 42, "FrameIcmp4 must be 42 bytes");


static void sendPingResponse(uint8_t *frame, int flen);

static constexpr struct {
    uint8_t type;
    void (*handler)(uint8_t *frame, int flen);
} registry[] = {
    {8, sendPingResponse}
};

void ICMP_process(uint8_t *frame, int flen) {
    // map headers
    const auto &packet = FrameIcmp4::from(frame);

    // verify destination
    if (isMyMAC(packet.eth.macDst))
        return;
    if (packet.ip4.dst != ipAddress)
        return;

    for (const auto &entry : registry) {
        if(packet.icmp.type == entry.type) {
            (*entry.handler)(frame, flen);
            break;
        }
    }
}

static void sendPingResponse(uint8_t *frame, const int flen) {
    // ICMP code must be zero
    if (FrameIcmp4::from(frame).icmp.code != 0)
        return;

    // get TX descriptor
    const int txDesc = NET_getTxDesc();
    const auto txFrame = NET_getTxBuff(txDesc);
    memcpy(txFrame, frame, flen);
    // map headers
    auto &packet = FrameIcmp4::from(txFrame);

    // modify ethernet frame header
    copyMAC(packet.eth.macDst, packet.eth.macSrc);

    // modify IP header
    packet.ip4.dst = packet.ip4.src;
    packet.ip4.src = ipAddress;

    // change type to echo response
    packet.icmp.type = 0;
    // touch up checksum
    if (packet.icmp.chksum[0] > 0xF7)
        ++packet.icmp.chksum[1];
    packet.icmp.chksum[0] += 0x08;

    // transmit response
    IPv4_finalize(txFrame, flen);
    NET_transmit(txDesc, flen);
}
