//
// Created by robert on 5/16/22.
//

#include <memory.h>
#include <strings.h>

#include "arp.h"
#include "eth.h"
#include "ip.h"
#include "../net.h"

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

#define ARP_FRAME_LEN (sizeof(struct HEADER_ETH) + sizeof(struct PAYLOAD_ARP_IP4) + 4)

#define MAX_REQUESTS (16)
volatile struct {
    uint32_t requestIp;
    CallbackARP requestCallback;
} requests[MAX_REQUESTS];

extern uint8_t *macAddress;

void makeArpIp4(
        uint8_t *packet,
        const uint8_t op,
        const uint8_t *macSrc,
        const uint8_t *ipSrc,
        const uint8_t *macTrg,
        const uint8_t *ipTrg
) {
    struct HEADER_ETH *header = (struct HEADER_ETH *) packet;
    packet += sizeof(struct HEADER_ETH);
    struct PAYLOAD_ARP_IP4 *payload = (struct PAYLOAD_ARP_IP4 *) packet;

    // init frame header
    ETH_initHeader(header);
    // ARP subtype
    header->ethType[1] = 0x06;

    payload->HTYPE[0] = 0x00;
    payload->HTYPE[1] = 0x01;
    payload->PTYPE[0] = 0x08;
    payload->PTYPE[1] = 0x00;
    payload->HLEN = 6;
    payload->PLEN = 4;
    payload->OPER[0] = 0x00;
    payload->OPER[1] = op;
    memcpy(payload->SHA, macSrc, 6);
    memcpy(payload->SPA, ipSrc, 4);
    memcpy(payload->THA, macTrg, 6);
    memcpy(payload->TPA, ipTrg, 4);
}

void ARP_poll() {

}

void ARP_process(uint8_t *packet) {
    struct PAYLOAD_ARP_IP4 *payload = (struct PAYLOAD_ARP_IP4 *) packet;
    if(payload->HTYPE[0] != 0x00) return;
    if(payload->HTYPE[1] != 0x01) return;
    if(payload->PTYPE[0] != 0x08) return;
    if(payload->PTYPE[1] != 0x00) return;
    if(payload->HLEN != 6) return;
    if(payload->PLEN != 4) return;
    if(payload->OPER[0] != 0) return;
    if(payload->OPER[1] == ARP_OP_REQUEST) {
        if(ipAddress) {
            uint32_t addr;
            memcpy(&addr, payload->TPA, 4);
            if (ipAddress == addr) {
                int txDesc = NET_getTxDesc();
                if(txDesc >= 0) {
                    uint8_t *packetTX = NET_getTxBuff(txDesc);
                    makeArpIp4(
                            packetTX, ARP_OP_REPLY,
                            macAddress, (uint8_t *) &ipAddress,
                            payload->SHA, payload->SPA
                    );
                    memcpy(((struct HEADER_ETH *)packetTX)->macDst, payload->SHA, 6);
                    NET_transmit(txDesc, ARP_FRAME_LEN);
                }
            }
        }
    }
    else if(payload->OPER[1] == ARP_OP_REPLY) {
        uint32_t addr;
        memcpy(&addr, payload->SPA, 4);
        if(addr != 0) {
            for(int i = 0; i < MAX_REQUESTS; i++) {
                if(requests[i].requestIp == addr) {
                    (*requests[i].requestCallback)(addr, payload->SHA);
                }
            }
        }
    }
}

void ARP_broadcastIP() {

}
