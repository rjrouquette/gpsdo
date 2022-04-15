//
// Created by robert on 4/15/22.
//

#ifndef GPSDO_DELAY_H
#define GPSDO_DELAY_H

#include <stdint.h>

__attribute__((always_inline))
inline void delay_cycles_1(void) {
    __asm volatile("nop");
}

__attribute__((always_inline))
inline void delay_cycles_2(void) {
    delay_cycles_1();
    delay_cycles_1();
}

__attribute__((always_inline))
inline void delay_cycles_4(void) {
    delay_cycles_2();
    delay_cycles_2();
}

__attribute__((always_inline))
inline void delay_loop(volatile uint32_t cnt) { while(cnt) --cnt; }

void delay_us(uint16_t delay);
void delay_ms(uint16_t delay);

#endif //GPSDO_DELAY_H
