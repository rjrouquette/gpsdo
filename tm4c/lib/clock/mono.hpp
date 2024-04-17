//
// Created by robert on 4/26/23.
//

#pragma once

#include <cstdint>
#include "../hw/timer.h"

static constexpr int MAX_RAW_INTV = 1u << 30;
static constexpr int CLK_FREQ = 125000000;
static constexpr int CLK_NANO = 1000000000 / CLK_FREQ;
#define TIMER_MONO (GPTM0)

// raw internal monotonic clock state
extern volatile uint32_t clkMonoInt;
extern volatile uint32_t clkMonoOff;
// timer tick capture of the PPS output
extern volatile uint32_t clkMonoPps;

namespace clock::monotonic {
    /**
     * Returns the current value of the system clock (1s resolution)
     * @return Raw 32-bit count of 1s ticks
     */
    uint32_t seconds();

    /**
     * Returns the current value of the system clock (~0.232ns resolution)
     * @return 64-bit fixed-point format (32.32)
     */
    uint64_t now();

    /**
     * Converts a raw monotonic timer value into a full precision timestamp (~0.232ns resolution)
     * @return 64-bit fixed-point format (32.32)
     */
    uint64_t fromRaw(uint32_t monoRaw);

    /**
     * Returns the raw value of the system clock timer
     */
    inline uint32_t raw() {
        return TIMER_MONO.TAV.raw;
    }
}
