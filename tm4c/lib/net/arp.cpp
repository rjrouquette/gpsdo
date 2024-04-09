//
// Created by robert on 5/16/22.
//

#include "arp.hpp"

#include "eth.hpp"
#include "ip.hpp"
#include "util.hpp"
#include "../net.hpp"
#include "../run.hpp"
#include "../clk/mono.h"

#include <strings.h>

#define ANNOUNCE_INTERVAL (60) // 60 seconds
#define MAX_REQUESTS (16)
#define REQUEST_EXPIRE (5) //  5 seconds

#define ARP_OP_REQUEST (0x0100)
#define ARP_OP_REPLY (0x0200)

#define ARP_FRAME_LEN (60)

struct [[gnu::packed]] HeaderArp4 {
    uint16_t HTYPE;
    uint16_t PTYPE;
    uint8_t HLEN;
    uint8_t PLEN;
    uint16_t OPER;
    uint8_t SHA[6];
    uint32_t SPA;
    uint8_t THA[6];
    uint32_t TPA;
};

static_assert(sizeof(HeaderArp4) == 28, "HeaderArp4 must be 28 bytes");

struct [[gnu::packed]] FrameArp4 : FrameEthernet {
    HeaderArp4 arp;

    static auto& from(void *frame) {
        return *static_cast<FrameArp4*>(frame);
    }

    static auto& from(const void *frame) {
        return *static_cast<const FrameArp4*>(frame);
    }
};

static_assert(sizeof(FrameArp4) == 42, "FrameArp4 must be 42 bytes");

struct ArpRequest {
    uint32_t expire;
    uint32_t remoteAddress;
    CallbackARP callback;
    void *ref;
};

ArpRequest requests[MAX_REQUESTS];

uint8_t macRouter[6];

static uint32_t nextAnnounce = 0;
static uint32_t lastRouterPoll = 0;


static void arpRouter(void *ref, uint32_t remoteAddress, const uint8_t *macAddress) {
    // retry if request timed-out
    if (isNullMAC(macAddress))
        ARP_refreshRouter();
    else
        copyMAC(macRouter, macAddress);
}

void ARP_refreshRouter() {
    lastRouterPoll = CLK_MONO_INT();
    ARP_request(ipRouter, arpRouter, nullptr);
}

void makeArpIp4(
    void *frame,
    const uint16_t op,
    const void *macTrg,
    const uint32_t ipTrg
) {
    bzero(frame, ARP_FRAME_LEN);
    // map headers
    auto &packet = FrameArp4::from(frame);
    // ARP frame type
    packet.eth.ethType = ETHTYPE_ARP;
    // ARP payload
    packet.arp.HTYPE = 0x0100;
    packet.arp.PTYPE = ETHTYPE_IP4;
    packet.arp.HLEN = 6;
    packet.arp.PLEN = 4;
    packet.arp.OPER = op;
    getMAC(packet.arp.SHA);
    packet.arp.SPA = ipAddress;
    copyMAC(packet.arp.THA, macTrg);
    packet.arp.TPA = ipTrg;
}

static void arpAnnounce() {
    // our IP address must be valid
    if (ipAddress == 0)
        return;
    // get TX descriptor
    const int txDesc = NET_getTxDesc();
    if (txDesc < 0)
        return;
    // create request frame
    const uint8_t wildCard[6] = {0, 0, 0, 0, 0, 0};
    uint8_t *packetTX = NET_getTxBuff(txDesc);
    makeArpIp4(
        packetTX, ARP_OP_REQUEST,
        wildCard, ipAddress
    );
    broadcastMAC(reinterpret_cast<HeaderEthernet*>(packetTX)->macDst);
    // transmit frame
    NET_transmit(txDesc, ARP_FRAME_LEN);
}

static void arpRun(void *ref) {
    // process request expiration
    const uint32_t now = CLK_MONO_INT();
    for (auto &request : requests) {
        // skip empty slots
        if (request.remoteAddress == 0)
            continue;
        // skip fresh requests
        if (static_cast<int32_t>(now - request.expire) <= 0)
            continue;
        // report expiration
        const uint8_t nullMac[6] = {0, 0, 0, 0, 0, 0};
        (*request.callback)(request.ref, request.remoteAddress, nullMac);
        request.remoteAddress = 0;
    }

    // link must be up to send packets
    if (!(NET_getPhyStatus() & PHY_STATUS_LINK))
        return;

    // announce presence
    if (static_cast<int32_t>(nextAnnounce - now) <= 0) {
        nextAnnounce = now + ANNOUNCE_INTERVAL;
        arpAnnounce();
    }

    // refresh router MAC address
    if ((now - lastRouterPoll) > ARP_MAX_AGE)
        ARP_refreshRouter();
}

void ARP_init() {
    // wake every 0.5 seconds
    runSleep(RUN_SEC >> 1, arpRun, nullptr);
}

void ARP_process(uint8_t *frame, const int flen) {
    // reject if packet is incorrect size
    if (flen != ARP_FRAME_LEN)
        return;
    // map header and payload
    const auto &packet = FrameArp4::from(frame);

    // verify payload fields
    if (packet.arp.HTYPE != 0x0100)
        return;
    if (packet.arp.PTYPE != ETHTYPE_IP4)
        return;
    if (packet.arp.HLEN != 6)
        return;
    if (packet.arp.PLEN != 4)
        return;
    // perform operation
    if (packet.arp.OPER == ARP_OP_REQUEST) {
        // send response
        if (ipAddress) {
            if (ipAddress == packet.arp.TPA) {
                const int txDesc = NET_getTxDesc();
                uint8_t *packetTX = NET_getTxBuff(txDesc);
                makeArpIp4(
                    packetTX, ARP_OP_REPLY,
                    packet.arp.SHA, packet.arp.SPA
                );
                copyMAC(reinterpret_cast<HeaderEthernet*>(packetTX)->macDst, packet.eth.macSrc);
                NET_transmit(txDesc, ARP_FRAME_LEN);
            }
        }
    }
    else if (packet.arp.OPER == ARP_OP_REPLY) {
        // discard if not directly addressed to us
        if (isMyMAC(packet.eth.macDst))
            return;
        // process reply
        if (packet.arp.SPA != 0) {
            for (auto &request : requests) {
                if (request.remoteAddress == packet.arp.SPA) {
                    (*request.callback)(request.ref, request.remoteAddress, packet.arp.SHA);
                    request.remoteAddress = 0;
                }
            }
        }
    }
}

int ARP_request(const uint32_t remoteAddress, const CallbackARP callback, void *ref) {
    const uint32_t now = CLK_MONO_INT();
    for (auto &request : requests) {
        // look for empty slot
        if (request.remoteAddress != 0)
            continue;
        // get TX descriptor
        const int txDesc = NET_getTxDesc();
        // create request frame
        const uint8_t wildCard[6] = {0, 0, 0, 0, 0, 0};
        uint8_t *packetTX = NET_getTxBuff(txDesc);
        makeArpIp4(
            packetTX, ARP_OP_REQUEST,
            wildCard, remoteAddress
        );
        broadcastMAC(reinterpret_cast<HeaderEthernet*>(packetTX)->macDst);
        // register callback
        request.callback = callback;
        request.ref = ref;
        request.remoteAddress = remoteAddress;
        request.expire = now + REQUEST_EXPIRE;
        // transmit frame
        NET_transmit(txDesc, ARP_FRAME_LEN);
        return 0;
    }
    return -1;
}
