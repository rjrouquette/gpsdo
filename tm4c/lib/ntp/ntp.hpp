//
// Created by robert on 4/27/23.
//

#pragma once

#include <cstdint>

#define NTP_REF_GPS (0x00535047u)

namespace ntp {
    void init();

    uint32_t refId();
}
