//
// Created by robert on 4/26/23.
//

#ifndef GPSDO_CLK_MONO_H
#define GPSDO_CLK_MONO_H

#include <stdint.h>

#define CLK_FREQ (125000000)


/**
 * Precision Clock Event Structure
 */
struct ClockEvent {
    // monotonic clock state
    uint32_t timer;
    uint32_t offset;
    uint32_t integer;
    // trimmed clock state
    int32_t trimRate;
    uint64_t trimRef;
    uint64_t trimOff;
    // tai clock state
    int32_t taiRate;
    uint64_t taiRef;
    uint64_t taiOff;
};

// raw internal monotonic clock state
extern volatile uint32_t clkMonoInt;
extern volatile uint32_t clkMonoOff;
// timer tick offset between the ethernet clock and monotonic clock
extern volatile int64_t clkMonoEth;
// pps edge capture state
extern volatile struct ClockEvent clkMonoPps;

/**
 * Initialize monotonic system clock
 */
void CLK_MONO_init();

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
 * Returns the timestamp of the most recent PPS edge capture
 * @return 64-bit fixed-point format (32.32)
 */
uint64_t CLK_MONO_PPS();


#endif //GPSDO_CLK_MONO_H
