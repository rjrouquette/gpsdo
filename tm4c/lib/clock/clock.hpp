//
// Created by robert on 4/27/23.
//

#pragma once

// MOSC = 25 MHz
static constexpr int MOSC_FREQ = 25000000;
// CLK = 125 MHz
static constexpr int CLK_FREQ = 125000000;
// nanoseconds per clock cycle
static constexpr int CLK_NANOS = 1000000000 / CLK_FREQ;

namespace clock {
    void initSystem();
    void init();
}
