//
// Created by robert on 4/26/23.
//

#ifndef GPSDO_CLK_TRIM_H
#define GPSDO_CLK_TRIM_H

#include <stdint.h>

extern volatile uint64_t clkTrimOffset;
extern volatile uint64_t clkTrimRef;
extern volatile int32_t clkTrimRate;

/**
 * Returns the current value of the trimmed system clock
 * @return 64-bit fixed-point format (32.32)
 */
uint64_t CLK_TRIM();

/**
 * Translate monotonic timestamp to trimmed timestamp
 * @param ts timestamp to translate (32.32)
 * @return 64-bit fixed-point format (32.32)
 */
uint64_t CLK_TRIM_fromMono(uint64_t ts);

/**
 * Trim system clock frequency
 * @param trim new trim rate (0.31 fixed-point)
 */
void CLK_TRIM_setTrim(int32_t trim);

/**
 * Get current trim rate for system clock frequency
 * @return current trim rate (0.31 fixed-point)
 */
int32_t CLK_TRIM_getTrim();


#endif //GPSDO_CLK_TRIM_H
