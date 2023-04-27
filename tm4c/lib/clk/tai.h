//
// Created by robert on 4/27/23.
//

#ifndef GPSDO_TAI_H
#define GPSDO_TAI_H

#include <stdint.h>

extern volatile uint64_t clkTaiOffset;
extern volatile uint64_t clkTaiRef;
extern volatile int32_t clkTaiRate;

/**
 * Returns the current value of the TAI clock
 * @return 64-bit fixed-point format (32.32)
 */
uint64_t CLK_TAI();

/**
 * Returns the timestamp of the most recent PPS edge capture
 * @return 64-bit fixed-point format (32.32)
 */
uint64_t CLK_TAI_PPS();

/**
 * Translate monotonic timestamp to TAI timestamp
 * @return 64-bit fixed-point format (32.32)
 */
uint64_t CLK_TAI_fromMono(uint64_t mono);

/**
 * Adjust fractional part of TAI reference
 * @param fraction fractional seconds by which to adjust alignment
 */
void CLK_TAI_align(int32_t fraction);

/**
 * Set integer part of TAI reference
 * @param seconds 32-bit
 */
void CLK_TAI_set(uint32_t seconds);

/**
 * Trim TAI clock frequency
 * @param trim new trim rate (0.31 fixed-point)
 */
void CLK_TAI_setTrim(int32_t trim);

/**
 * Get current trim rate for TAI clock frequency
 * @return current trim rate (0.31 fixed-point)
 */
int32_t CLK_TAI_getTrim();

#endif //GPSDO_TAI_H
