//
// Created by robert on 4/28/23.
//

#pragma once

#include <cstdint>
#include "../net/ip.hpp"
#include "../net/udp.hpp"

#define NTP4_SIZE (UDP_DATA_OFFSET + 48)

#define NTP_PORT_SRV (123)
#define NTP_PORT_CLI (12345)

#define NTP_MODE_CLI (3)
#define NTP_MODE_SRV (4)

#define NTP_CLK_PREC (-27)

#define NTP_UTC_OFFSET (0x83AA7E8000000000ull)

struct [[gnu::packed]] HEADER_NTP {
    uint16_t mode    : 3;
    uint16_t version : 3;
    uint16_t status  : 2;
    uint8_t stratum;
    uint8_t poll;
    int8_t precision;
    uint32_t rootDelay;
    uint32_t rootDispersion;
    uint32_t refID;
    uint64_t refTime;
    uint64_t origTime;
    uint64_t rxTime;
    uint64_t txTime;
};

static_assert(sizeof(HEADER_NTP) == 48, "HEADER_NTP4 must be 48 bytes");
