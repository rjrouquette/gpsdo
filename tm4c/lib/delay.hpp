//
// Created by robert on 4/15/22.
//

#pragma once

#include <cstdint>

/**
 * Delay one instruction cycle
 */
__attribute__((always_inline))
inline void delay_cycles_1() {
    __asm volatile("nop");
}

/**
 * Delay two instruction cycles
 */
__attribute__((always_inline))
inline void delay_cycles_2() {
    delay_cycles_1();
    delay_cycles_1();
}

/**
 * Delay four instruction cycles
 */
__attribute__((always_inline))
inline void delay_cycles_4() {
    delay_cycles_2();
    delay_cycles_2();
}

/**
 * Delay with microsecond precsion
 * @param delay in microseconds
 */
void delay_us(uint16_t delay);

/**
 * Delay with millisecond precision
 * @param delay in milliseconds
 */
void delay_ms(uint16_t delay);
