//
// Created by robert on 4/27/23.
//

#pragma once

#include <cstdint>

namespace ntp {
    static constexpr uint32_t REF_ID_GPS = 0x00535047u;

    void init();

    uint32_t refId();
}
