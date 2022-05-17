//
// Created by robert on 5/16/22.
//

#include "arp.h"
#include "eth.h"

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

    for(int i = 0; i < 6; i++) {
        payload->SHA[i] = macSrc[i];
        payload->THA[i] = macTrg[i];
    }

    for(int i = 0; i < 4; i++) {
        payload->SPA[i] = ipSrc[i];
        payload->TPA[i] = ipTrg[i];
    }
}

void ARP_poll() {

}

void APR_process(uint8_t *packet) {
    struct HEADER_ETH *header = (struct HEADER_ETH *) packet;
    packet += sizeof(struct HEADER_ETH);
    struct PAYLOAD_ARP_IP4 *payload = (struct PAYLOAD_ARP_IP4 *) packet;
    if(payload->HTYPE[0] != 0x00) return;
    if(payload->HTYPE[1] != 0x01) return;
    if(payload->PTYPE[0] != 0x08) return;
    if(payload->PTYPE[1] != 0x00) return;
    if(payload->HLEN != 6) return;
    if(payload->PLEN != 4) return;
    if(payload->OPER[0] != 0) return;
    if(payload->OPER[1] == ARP_OP_REQUEST) {
        // TODO process request
    }
    else if(payload->OPER[1] == ARP_OP_REPLY) {
        // TODO process reply
    }
}
