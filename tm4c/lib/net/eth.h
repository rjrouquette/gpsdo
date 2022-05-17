//
// Created by robert on 5/16/22.
//

#ifndef GPSDO_ETH_H
#define GPSDO_ETH_H

#include <stdint.h>

#ifndef PACKED
#define PACKED __attribute__((packed))
#endif

struct PACKED HEADER_ETH {
    uint8_t macDst[6];
    uint8_t macSrc[6];
    uint8_t ethType[2];
};

struct PACKED HEADER_ETH_VLAN {
    uint8_t macDst[6];
    uint8_t macSrc[6];
    uint8_t vlanTag[4];
    uint8_t ethType[2];
};

void ETH_initHeader(struct HEADER_ETH *header);
void ETH_initHeaderVlan(struct HEADER_ETH_VLAN *header, uint16_t vlan);
void ETH_broadcastMAC(uint8_t *mac);

#endif //GPSDO_ETH_H
