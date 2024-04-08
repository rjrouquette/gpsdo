//
// Created by robert on 5/16/22.
//

#ifndef GPSDO_ETH_H
#define GPSDO_ETH_H

#ifdef __cplusplus
extern "C" {
#else
#define static_assert _Static_assert
#endif

#include <stdint.h>

#ifndef PACKED
#define PACKED __attribute__((packed))
#endif

typedef struct PACKED HEADER_ETH {
    uint8_t macDst[6];
    uint8_t macSrc[6];
    uint16_t ethType;
} HEADER_ETH;
static_assert(sizeof(struct HEADER_ETH) == 14, "HEADER_ETH must be 14 bytes");

#define ETHTYPE_ARP (0x0608)
#define ETHTYPE_IP4 (0x0008)
#define ETHTYPE_IP6 (0xDD86)
#define ETHTYPE_PTP (0xF788)

#ifdef __cplusplus
}
#endif

#endif //GPSDO_ETH_H
