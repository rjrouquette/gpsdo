//
// Created by robert on 4/15/22.
//

#include "delay.hpp"
#include "clock/mono.hpp"

static constexpr uint32_t CLK_KHZ = CLK_FREQ / 1000;
static constexpr uint32_t CLK_MHZ = CLK_KHZ / 1000;
static constexpr uint32_t DELAY_MAX_MS = MAX_RAW_INTV / CLK_KHZ;

void delay::micros(const uint16_t delay) {
    // get current time
    uint32_t target = clock::monotonic::raw();
    // compute target count for delay
    target += static_cast<uint32_t>(delay) * CLK_MHZ;
    // wait for timer to cross target
    while(static_cast<int32_t>(target - clock::monotonic::raw()) > 0);
}

void delay::millis(uint16_t delay) {
    // get current time
    uint32_t target = clock::monotonic::raw();
    // coarse delay
    while(delay > DELAY_MAX_MS) {
        // compute target count for delay
        delay -= DELAY_MAX_MS;
        target += DELAY_MAX_MS * CLK_KHZ;
        // wait for timer to cross target
        while(static_cast<int32_t>(target - clock::monotonic::raw()) > 0);
    }
    // remainder of delay
    target += static_cast<uint32_t>(delay) * CLK_KHZ;
    // wait for timer to cross target
    while(static_cast<int32_t>(target - clock::monotonic::raw()) > 0);
}
