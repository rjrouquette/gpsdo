//
// Created by robert on 5/28/22.
//

#pragma once

#include <cstdint>

namespace gps {
    void init();

    float locLat();
    float locLon();
    float locAlt();

    int clkBias();
    int clkDrift();
    int accTime();
    int accFreq();

    int hasFix();

    uint64_t taiEpochUpdate();
    uint32_t taiEpoch();
    int taiOffset();
}
