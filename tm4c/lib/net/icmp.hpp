//
// Created by robert on 5/19/22.
//

#pragma once

#include <cstdint>

struct [[gnu::packed]] HEADER_ICMP4 {
    uint8_t type;
    uint8_t code;
    uint8_t chksum[2];
    uint8_t misc[4];
};
static_assert(sizeof(HEADER_ICMP4) == 8, "HEADER_ICMP4 must be 8 bytes");


void ICMP_process(uint8_t *frame, int flen);
