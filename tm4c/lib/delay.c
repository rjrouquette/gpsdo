//
// Created by robert on 4/15/22.
//

#include "../hw/timer.h"
#include "delay.h"

#define CLK_KHZ (125000)    // 125 MHz
#define CLK_MHZ (125)       // 125 MHz
#define DELAY_MAX (1<<14)   // 16.384 ms

void delay_us(uint16_t delay) {
    // get current time
    uint32_t target = GPTM0.TAV.raw;
    // compute target count for delay
    target += ((uint32_t)delay) * CLK_MHZ;
    // wait for timer to cross target
    while(((int32_t)(target - GPTM0.TAV.raw)) > 0);
}

void delay_ms(uint16_t delay) {
    // get current time
    uint32_t target = GPTM0.TAV.raw;
    // coarse delay
    while(delay > DELAY_MAX) {
        // compute target count for delay
        delay -= DELAY_MAX;
        target += DELAY_MAX * CLK_KHZ;
        // wait for timer to cross target
        while(((int32_t)(target - GPTM0.TAV.raw)) > 0);
    }
    // remainder of delay
    target += ((uint32_t)delay) * CLK_KHZ;
    // wait for timer to cross target
    while(((int32_t)(target - GPTM0.TAV.raw)) > 0);
}
