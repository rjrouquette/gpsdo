//
// Created by robert on 5/16/22.
//

#pragma once

#include <cstdint>

typedef struct [[gnu::packed]] HEADER_ETH {
    uint8_t macDst[6];
    uint8_t macSrc[6];
    uint16_t ethType;
} HEADER_ETH;
static_assert(sizeof(HEADER_ETH) == 14, "HEADER_ETH must be 14 bytes");

#define ETHTYPE_ARP (0x0608)
#define ETHTYPE_IP4 (0x0008)
#define ETHTYPE_IP6 (0xDD86)
#define ETHTYPE_PTP (0xF788)
