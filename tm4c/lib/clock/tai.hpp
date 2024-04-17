//
// Created by robert on 4/27/23.
//

#pragma once

#include <cstdint>

extern volatile uint64_t clkTaiUtcOffset;

extern volatile uint64_t clkTaiOffset;
extern volatile uint64_t clkTaiRef;
extern volatile int32_t clkTaiRate;


namespace clock::tai {
    /**
     * Returns the current value of the TAI clock
     * @return 64-bit fixed-point format (32.32)
     */
    uint64_t now();

    /**
     * Translate monotonic timestamp to TAI timestamp
     * @param ts timestamp to translate (32.32)
     * @return 64-bit fixed-point format (32.32)
     */
    uint64_t fromMono(uint64_t ts);

    /**
     * Adjust TAI reference time
     * @param delta seconds in 64-bit fixed-point format (31.32)
     */
    void adjust(int64_t delta);

    /**
     * Trim TAI clock frequency
     * @param trim new trim rate (0.31 fixed-point)
     */
    void setTrim(int32_t trim);

    /**
     * Get current trim rate for TAI clock frequency
     * @return current trim rate (0.31 fixed-point)
     */
    int32_t getTrim();
}
