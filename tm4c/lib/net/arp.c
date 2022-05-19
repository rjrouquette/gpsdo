//
// Created by robert on 5/16/22.
//

#include <memory.h>
#include <strings.h>

#include "arp.h"
#include "eth.h"
#include "ip.h"
#include "../net.h"
#include "util.h"

#define ARP_OP_REQUEST (1)
#define ARP_OP_REPLY (2)

struct PACKED PAYLOAD_ARP_IP4 {
    uint8_t HTYPE[2];
    uint8_t PTYPE[2];
    uint8_t HLEN;
    uint8_t PLEN;
    uint8_t OPER[2];
    uint8_t SHA[6];
    uint8_t SPA[4];
    uint8_t THA[6];
    uint8_t TPA[4];
};

#define ARP_FRAME_LEN (64)

#define MAX_REQUESTS (16)
volatile struct {
    uint32_t remoteAddress;
    CallbackARP callback;
} requests[MAX_REQUESTS];

void makeArpIp4(
        void *packet,
        const uint8_t op,
        const volatile void *macSrc,
        const volatile void *ipSrc,
        const volatile void *macTrg,
        const volatile void *ipTrg
) {
//    memset(packet, 0, ARP_FRAME_LEN);

    struct HEADER_ETH *header = (struct HEADER_ETH *) packet;
    struct PAYLOAD_ARP_IP4 *payload = (struct PAYLOAD_ARP_IP4 *) (header + 1);

    // ARP frame type
    header->ethType[0] = 0x08;
    header->ethType[1] = 0x06;

    // ARP payload
    payload->HTYPE[0] = 0x00;
    payload->HTYPE[1] = 0x01;
    payload->PTYPE[0] = 0x08;
    payload->PTYPE[1] = 0x00;
    payload->HLEN = 6;
    payload->PLEN = 4;
    payload->OPER[0] = 0x00;
    payload->OPER[1] = op;
    copyMAC(payload->SHA, macSrc);
    copyIPv4(payload->SPA, ipSrc);
    copyMAC(payload->THA, macTrg);
    copyIPv4(payload->TPA, ipTrg);
}

void ARP_poll() {

}

void ARP_process(uint8_t *frame, int flen) {
    // reject if packet is incorrect size
    if(flen != ARP_FRAME_LEN) return;
    // map header and payload
    struct HEADER_ETH *header = (struct HEADER_ETH *) frame;
    struct PAYLOAD_ARP_IP4 *payload = (struct PAYLOAD_ARP_IP4 *) (header + 1);
    // verify payload fields
    if(payload->HTYPE[0] != 0x00) return;
    if(payload->HTYPE[1] != 0x01) return;
    if(payload->PTYPE[0] != 0x08) return;
    if(payload->PTYPE[1] != 0x00) return;
    if(payload->HLEN != 6) return;
    if(payload->PLEN != 4) return;
    if(payload->OPER[0] != 0) return;
    // perform operation
    if(payload->OPER[1] == ARP_OP_REQUEST) {
        // send response
        if(ipAddress) {
            uint32_t addr;
            copyIPv4(&addr, payload->TPA);
            if (ipAddress == addr) {
                uint8_t myMac[6];
                getMAC(myMac);
                int txDesc = NET_getTxDesc();
                if(txDesc >= 0) {
                    uint8_t *packetTX = NET_getTxBuff(txDesc);
                    makeArpIp4(
                            packetTX, ARP_OP_REPLY,
                            myMac, &ipAddress,
                            payload->SHA, payload->SPA
                    );
                    copyMAC(((struct HEADER_ETH *)packetTX)->macDst, header->macSrc);
                    NET_transmit(txDesc, ARP_FRAME_LEN);
                }
            }
        }
    }
    else if(payload->OPER[1] == ARP_OP_REPLY) {
        // process reply
        uint32_t addr;
        copyIPv4(&addr, payload->SPA);
        if(addr != 0) {
            for(int i = 0; i < MAX_REQUESTS; i++) {
                if(requests[i].remoteAddress == addr) {
                    (*requests[i].callback)(addr, payload->SHA);
                }
            }
        }
    }
}

int ARP_request(uint32_t remoteAddress, CallbackARP callback) {
    for(int i = 0; i < MAX_REQUESTS; i++) {
        // look for empty slot
        if(requests[i].remoteAddress != 0)
            continue;
        // get TX descriptor
        int txDesc = NET_getTxDesc();
        if(txDesc < 0) return -1;
        // create request frame
        uint8_t myMac[6];
        getMAC(myMac);
        uint8_t wildCard[6] = { 0, 0, 0, 0, 0, 0 };
        uint8_t *packetTX = NET_getTxBuff(txDesc);
        makeArpIp4(
                packetTX, ARP_OP_REQUEST,
                myMac, (uint8_t *) &ipAddress,
                wildCard, (uint8_t *) &remoteAddress
        );
        broadcastMAC(((struct HEADER_ETH *)packetTX)->macDst);
        // register callback
        requests[i].callback = callback;
        requests[i].remoteAddress = remoteAddress;
        // transmit frame
        NET_transmit(txDesc, ARP_FRAME_LEN);
        return 0;
    }
    return -1;
}

void ARP_broadcastIP() {

}
