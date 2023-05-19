//
// Created by robert on 5/16/22.
//

#ifndef GPSDO_ETH_H
#define GPSDO_ETH_H

#include <stdint.h>

#ifndef PACKED
#define PACKED __attribute__((packed))
#endif

typedef struct PACKED HEADER_ETH {
    uint8_t macDst[6];
    uint8_t macSrc[6];
    uint16_t ethType;
} HEADER_ETH;
_Static_assert(sizeof(struct HEADER_ETH) == 14, "FRAME_ETH must be 14 bytes");

#define ETHTYPE_ARP (0x0608)
#define ETHTYPE_IPv4 (0x0008)

#endif //GPSDO_ETH_H
