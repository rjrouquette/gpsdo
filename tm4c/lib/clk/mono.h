//
// Created by robert on 4/26/23.
//

#ifndef GPSDO_CLK_MONO_H
#define GPSDO_CLK_MONO_H

#include <stdint.h>
#include "../hw/timer.h"

#define CLK_FREQ (100000000)
#define CLK_NANO (1000000000 / CLK_FREQ)
#define TIMER_MONO (GPTM0)

/**
 * Precision Clock Event Structure
 */
struct ClockEvent {
    // monotonic clock state
    uint32_t timer;
    uint32_t offset;
    uint32_t integer;
    // compensated clock state
    int32_t compRate;
    uint64_t compRef;
    uint64_t compOff;
    // tai clock state
    int32_t taiRate;
    uint64_t taiRef;
    uint64_t taiOff;
};

// raw internal monotonic clock state
extern volatile uint32_t clkMonoInt;
extern volatile uint32_t clkMonoOff;
// timer tick offset between the ethernet clock and monotonic clock
extern volatile uint32_t clkMonoEth;
// timer tick capture of the PPS output
extern volatile uint32_t clkMonoPps;
// pps edge capture state
extern volatile struct ClockEvent clkMonoPpsEvent;

/**
 * Returns the current value of the system clock (1s resolution)
 * @return Raw 32-bit count of 1s ticks
 */
uint32_t CLK_MONO_INT();

/**
 * Returns the current value of the system clock (~0.232ns resolution)
 * @return 64-bit fixed-point format (32.32)
 */
uint64_t CLK_MONO();

/**
 * Returns the raw value of the system clock timer
 */
#define CLK_MONO_RAW (TIMER_MONO.TAV.raw)


#endif //GPSDO_CLK_MONO_H
