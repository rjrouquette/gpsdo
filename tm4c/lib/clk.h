//
// Created by robert on 4/15/22.
//

#ifndef GPSDO_CLK_H
#define GPSDO_CLK_H

#include <stdint.h>

/**
 * Initialize system clock and timers
 */
void CLK_init();

/**
 * Returns the current value of the system clock (1s resolution)
 * @return Raw 32-bit count of 1s ticks
 */
uint32_t CLK_MONOTONIC_INT();

/**
 * Returns the current value of the system clock (~0.232ns resolution)
 * @return 64-bit fixed-point format (32.32)
 */
uint64_t CLK_MONOTONIC();

/**
 * Returns the current value of TAI clock (1s resolution)
 * @return 32-bit count of 1s ticks
 */
uint32_t CLK_TAI_INT();

/**
 * Returns the current value of TAI clock (~0.232ns resolution)
 * @return 64-bit fixed-point format (32.32)
 */
uint64_t CLK_TAI();

/**
 * Trim fractional part of TAI reference
 * @param nanos nanoseconds by which to trim alignment
 */
void CLK_TAI_align(int32_t nanos);

/**
 * Set integer part of TAI reference
 * @param seconds 32-bit
 */
void CLK_TAI_set(uint32_t seconds);

/**
 * Trim TAI clock frequency
 * @param trim new trim rate
 * @return actual rate applied
 */
float CLK_TAI_trim(float trim);

/**
 * Convert nanoseconds into fractional second
 * @param nanos nanosecond value to convert
 * @return 32-bit fixed-point format (0.32)
 */
uint32_t nanosToFrac(uint32_t nanos);

#endif //GPSDO_CLK_H
