//
// Created by robert on 5/16/22.
//

#include <strings.h>
#include "../clk/mono.h"
#include "../net.h"
#include "../run.h"
#include "arp.h"
#include "eth.h"
#include "ip.h"
#include "util.h"

#define ANNOUNCE_INTERVAL (60) // 60 seconds
#define MAX_REQUESTS (16)
#define REQUEST_EXPIRE (5) //  5 seconds

typedef struct ArpRequest {
    uint32_t expire;
    uint32_t remoteAddress;
    CallbackARP callback;
    void *ref;
} ArpRequest;
ArpRequest requests[MAX_REQUESTS];

uint8_t macRouter[6];

static uint32_t nextAnnounce = 0;
static uint32_t lastRouterPoll = 0;


static void arpRouter(void *ref, uint32_t remoteAddress, uint8_t *macAddress) {
    // retry if request timed-out
    if(isNullMAC(macAddress))
        ARP_refreshRouter();
    else
        copyMAC(macRouter, macAddress);
}

void ARP_refreshRouter() {
    lastRouterPoll = CLK_MONO_INT();
    ARP_request(ipRouter, arpRouter, NULL);
}

void makeArpIp4(
        void *packet,
        const uint16_t op,
        const void *macTrg,
        const uint32_t ipTrg
) {
    bzero(packet, ARP_FRAME_LEN);

    HEADER_ETH *header = (HEADER_ETH *) packet;
    ARP_IP4 *payload = (ARP_IP4 *) (header + 1);

    // ARP frame type
    header->ethType = ETHTYPE_ARP;
    // ARP payload
    payload->HTYPE = 0x0100;
    payload->PTYPE = ETHTYPE_IP4;
    payload->HLEN = 6;
    payload->PLEN = 4;
    payload->OPER = op;
    getMAC(payload->SHA);
    payload->SPA = ipAddress;
    copyMAC(payload->THA, macTrg);
    payload->TPA = ipTrg;
}

static void arpAnnounce() {
    // our IP address must be valid
    if(ipAddress == 0) return;
    // get TX descriptor
    int txDesc = NET_getTxDesc();
    if(txDesc < 0) return;
    // create request frame
    uint8_t wildCard[6] = { 0, 0, 0, 0, 0, 0 };
    uint8_t *packetTX = NET_getTxBuff(txDesc);
    makeArpIp4(
            packetTX, ARP_OP_REQUEST,
            wildCard, ipAddress
    );
    broadcastMAC(((HEADER_ETH *)packetTX)->macDst);
    // transmit frame
    NET_transmit(txDesc, ARP_FRAME_LEN);
}

static void arpRun() {
    // process request expiration
    const uint32_t now = CLK_MONO_INT();
    for(int i = 0; i < MAX_REQUESTS; i++) {
        ArpRequest *request = requests + i;
        // skip empty slots
        if(request->remoteAddress == 0) continue;
        // skip fresh requests
        if(((int32_t) (now - request->expire)) <= 0) continue;
        // report expiration
        uint8_t nullMac[6] = {0,0,0,0,0,0};
        (*(request->callback))(request->ref, request->remoteAddress, nullMac);
        bzero(request, sizeof(ArpRequest));
    }

    // link must be up to send packets
    if(!(NET_getPhyStatus() & PHY_STATUS_LINK))
        return;

    // announce presence
    if(((int32_t)(nextAnnounce - now)) <= 0) {
        nextAnnounce = now + ANNOUNCE_INTERVAL;
        arpAnnounce();
    }

    // refresh router MAC address
    if((now - lastRouterPoll) > ARP_MAX_AGE)
        ARP_refreshRouter();
}

void ARP_init() {
    // wake every 0.5 seconds
    runSleep(1u << (32 - 1), arpRun, NULL);
}

void ARP_process(uint8_t *frame, int flen) {
    // reject if packet is incorrect size
    if(flen != ARP_FRAME_LEN) return;
    // map header and payload
    HEADER_ETH *header = (HEADER_ETH *) frame;
    ARP_IP4 *payload = (ARP_IP4 *) (header + 1);
    // verify payload fields
    if(payload->HTYPE != 0x0100) return;
    if(payload->PTYPE != ETHTYPE_IP4) return;
    if(payload->HLEN != 6) return;
    if(payload->PLEN != 4) return;
    // perform operation
    if(payload->OPER == ARP_OP_REQUEST) {
        // send response
        if(ipAddress) {
            if (ipAddress == payload->TPA) {
                int txDesc = NET_getTxDesc();
                if(txDesc >= 0) {
                    uint8_t *packetTX = NET_getTxBuff(txDesc);
                    makeArpIp4(
                            packetTX, ARP_OP_REPLY,
                            payload->SHA, payload->SPA
                    );
                    copyMAC(((HEADER_ETH *)packetTX)->macDst, header->macSrc);
                    NET_transmit(txDesc, ARP_FRAME_LEN);
                }
            }
        }
    }
    else if(payload->OPER == ARP_OP_REPLY) {
        // discard if not directly addressed to us
        if(isMyMAC(header->macDst)) return;
        // process reply
        if(payload->SPA != 0) {
            for(int i = 0; i < MAX_REQUESTS; i++) {
                if(requests[i].remoteAddress == payload->SPA) {
                    (*requests[i].callback)(requests[i].ref, requests[i].remoteAddress, payload->SHA);
                    bzero((void *) (requests + i), sizeof(ArpRequest));
                }
            }
        }
    }
}

int ARP_request(uint32_t remoteAddress, CallbackARP callback, void *ref) {
    uint32_t now = CLK_MONO_INT();
    for(int i = 0; i < MAX_REQUESTS; i++) {
        // look for empty slot
        if(requests[i].remoteAddress != 0)
            continue;
        // get TX descriptor
        int txDesc = NET_getTxDesc();
        if(txDesc < 0) return -1;
        // create request frame
        uint8_t wildCard[6] = { 0, 0, 0, 0, 0, 0 };
        uint8_t *packetTX = NET_getTxBuff(txDesc);
        makeArpIp4(
                packetTX, ARP_OP_REQUEST,
                wildCard, remoteAddress
        );
        broadcastMAC(((HEADER_ETH *)packetTX)->macDst);
        // register callback
        requests[i].callback = callback;
        requests[i].ref = ref;
        requests[i].remoteAddress = remoteAddress;
        requests[i].expire = now + REQUEST_EXPIRE;
        // transmit frame
        NET_transmit(txDesc, ARP_FRAME_LEN);
        return 0;
    }
    return -1;
}
