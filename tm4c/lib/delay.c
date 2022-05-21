//
// Created by robert on 4/15/22.
//

#include "../hw/sys.h"
#include "delay.h"

#define CLK_KHZ (125000)
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
    // get current time
    uint32_t target = STCURRENT.CURRENT;
    // coarse delay
    while(delay > 64) {
        // compute target count for delay
        delay -= 64;
        target -= 64 * CLK_KHZ;
        // wait for timer to cross target
        while ((target - STCURRENT.CURRENT) & (1u << 23u));
    }
    // final delay
    if(delay) {
        target -= delay * CLK_KHZ;
        // wait for timer to cross target
        while ((target - STCURRENT.CURRENT) & (1u << 23u));
    }
}
