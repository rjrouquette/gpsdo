//
// Created by robert on 5/16/22.
//

#include "eth.h"

void ETH_initHeader(struct HEADER_ETH *header) {
    // set type as Ethernet II
    header->ethType[0] = 0x08;
    header->ethType[1] = 0x00;
}

void ETH_initHeaderVlan(struct HEADER_ETH_VLAN *header, uint16_t vlan) {
    // set VLAN tag
    header->vlanTag[0] = 0x81;
    header->vlanTag[1] = 0x00;
    header->vlanTag[2] = (vlan >> 8) & 0xFF;
    header->vlanTag[3] = vlan & 0xFF;
    // set type as Ethernet II
    header->ethType[0] = 0x08;
    header->ethType[1] = 0x00;
}

void ETH_broadcastMAC(uint8_t *mac) {
    mac[0] = 0xFF;
    mac[1] = 0xFF;
    mac[2] = 0xFF;
    mac[3] = 0xFF;
    mac[4] = 0xFF;
    mac[5] = 0xFF;
}

int ETH_isARP(const uint8_t *ethType) {
    return ethType[0] == 0x08 && ethType[1] == 0x06;
}
