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
 * Returns the current value of TAI clock (~0.232ns resolution)
 * @return 64-bit fixed-point format (32.32)
 */
uint64_t CLK_TAI();

void CLK_setFracTAI(uint32_t offset);

#endif //GPSDO_CLK_H
