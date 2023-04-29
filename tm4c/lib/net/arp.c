//
// Created by robert on 5/16/22.
//

#include <strings.h>
#include "../clk/mono.h"
#include "../net.h"
#include "arp.h"
#include "eth.h"
#include "ip.h"
#include "util.h"

#define ANNOUNCE_INTERVAL (60) // 60 seconds
#define MAX_REQUESTS (16)
#define REQUEST_EXPIRE (5) //  5 seconds

volatile struct {
    uint32_t expire;
    uint32_t remoteAddress;
    CallbackARP callback;
    void *ref;
} requests[MAX_REQUESTS];

static uint32_t nextAnnounce = 0;


void makeArpIp4(
        void *packet,
        const uint16_t op,
        const volatile void *macTrg,
        const volatile uint32_t ipTrg
) {
    bzero(packet, ARP_FRAME_LEN);

    struct FRAME_ETH *header = (struct FRAME_ETH *) packet;
    struct PAYLOAD_ARP_IP4 *payload = (struct PAYLOAD_ARP_IP4 *) (header + 1);

    // ARP frame type
    header->ethType = ETHTYPE_ARP;
    // ARP payload
    payload->HTYPE = 0x0100;
    payload->PTYPE = ETHTYPE_IPv4;
    payload->HLEN = 6;
    payload->PLEN = 4;
    payload->OPER = op;
    getMAC(payload->SHA);
    payload->SPA = ipAddress;
    copyMAC(payload->THA, macTrg);
    payload->TPA = ipTrg;
}

void ARP_run() {
    const uint32_t now = CLK_MONO_INT();
    if(((int32_t)(nextAnnounce - now)) <= 0) {
        nextAnnounce = now + ANNOUNCE_INTERVAL;
        ARP_announce();
    }
    // expire requests
    for(int i = 0; i < MAX_REQUESTS; i++) {
        // skip empty slots
        if(requests[i].remoteAddress == 0) continue;
        // skip fresh requests
        if(((int32_t) (now - requests[i].expire) <= 0)) continue;
        // cancel request
        uint8_t nullMac[6] = {0,0,0,0,0,0};
        (*requests[i].callback)(requests[i].ref, requests[i].remoteAddress, nullMac);
        requests[i].remoteAddress = 0;
    }
}

void ARP_process(uint8_t *frame, int flen) {
    // reject if packet is incorrect size
    if(flen != ARP_FRAME_LEN) return;
    // map header and payload
    struct FRAME_ETH *header = (struct FRAME_ETH *) frame;
    struct PAYLOAD_ARP_IP4 *payload = (struct PAYLOAD_ARP_IP4 *) (header + 1);
    // verify payload fields
    if(payload->HTYPE != 0x0100) return;
    if(payload->PTYPE != ETHTYPE_IPv4) return;
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
                    copyMAC(((struct FRAME_ETH *)packetTX)->macDst, header->macSrc);
                    NET_transmit(txDesc, ARP_FRAME_LEN);
                }
            }
        }
    }
    else if(payload->OPER == ARP_OP_REPLY) {
        // process reply
        if(payload->SPA != 0) {
            for(int i = 0; i < MAX_REQUESTS; i++) {
                if(requests[i].remoteAddress == payload->SPA) {
                    (*requests[i].callback)(requests[i].ref, payload->SPA, payload->SHA);
                    requests[i].remoteAddress = 0;
                }
            }
        }
    }
}

int ARP_request(uint32_t remoteAddress, CallbackARP callback, volatile void *ref) {
    uint32_t now = CLK_MONO_INT();
    for(int i = 0; i < MAX_REQUESTS; i++) {
        // look for empty or expired slot
        if(requests[i].remoteAddress != 0 && ((int32_t) (now - requests[i].expire) > 0))
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
        broadcastMAC(((struct FRAME_ETH *)packetTX)->macDst);
        // register callback
        requests[i].callback = callback;
        requests[i].ref = (void *) ref;
        requests[i].remoteAddress = remoteAddress;
        requests[i].expire = now + REQUEST_EXPIRE;
        // transmit frame
        NET_transmit(txDesc, ARP_FRAME_LEN);
        return 0;
    }
    return -1;
}

void ARP_announce() {
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
    broadcastMAC(((struct FRAME_ETH *)packetTX)->macDst);
    // transmit frame
    NET_transmit(txDesc, ARP_FRAME_LEN);
}
