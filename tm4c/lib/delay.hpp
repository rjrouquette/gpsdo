//
// Created by robert on 4/15/22.
//

#pragma once

#include <cstdint>

namespace delay {
    /**
     * Pause execution for an exact number of instruction cycles by inserting "nop" instrcutions.
     * @param count number of instruction cycles to wait
     */
    [[gnu::always_inline, gnu::optimize("unroll-loops")]]
    inline void cycles(int count) {
        for(int i = 0; i < count; ++i)
            __asm volatile("nop");
    }

    /**
     * Pause with microsecond precsion
     * @param delay in microseconds
     */
    void micros(uint16_t delay);

    /**
     * Pause with millisecond precision
     * @param delay in milliseconds
     */
    void millis(uint16_t delay);
}
