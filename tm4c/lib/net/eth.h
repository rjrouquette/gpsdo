//
// Created by robert on 5/16/22.
//

#ifndef GPSDO_ETH_H
#define GPSDO_ETH_H

#include <stdint.h>

#ifndef PACKED
#define PACKED __attribute__((packed))
#endif

struct PACKED FRAME_ETH {
    uint8_t macDst[6];
    uint8_t macSrc[6];
    uint16_t ethType;
};
#ifdef __cplusplus
static_assert(sizeof(struct FRAME_ETH) == 14, "FRAME_ETH must be 14 bytes");
#else
_Static_assert(sizeof(struct FRAME_ETH) == 14, "FRAME_ETH must be 14 bytes");
#endif

#define ETHTYPE_ARP (0x0608)
#define ETHTYPE_IPv4 (0x0008)

#endif //GPSDO_ETH_H
