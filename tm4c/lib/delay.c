//
// Created by robert on 4/15/22.
//

#include "../hw/sys.h"
#include "delay.h"

#define CLK_MHZ (125)

void delay_us(uint16_t delay) {
    // get current time
    uint32_t target = STCURRENT.CURRENT;
    // compute target count for delay
    target -= ((uint32_t)delay) * CLK_MHZ;
    // wait for timer to cross target
    while((target - STCURRENT.CURRENT) & (1u << 23u));
}

void delay_ms(uint16_t delay) {
    // coarse delay
    while(delay > 50) {
        delay -= 50;
        delay_us(50000);
    }
    // moderate delay
    while(delay > 7) {
        delay -= 7;
        delay_us(7000);
    }
    // fine delay
    while(delay) {
        --delay;
        delay_us(1000);
    }
}
