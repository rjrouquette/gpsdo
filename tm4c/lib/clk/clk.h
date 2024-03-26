//
// Created by robert on 4/27/23.
//

#ifndef GPSDO_CLK_H
#define GPSDO_CLK_H

#include <stdint.h>

void CLK_initSys();
void CLK_init();

/**
 * Returns the timestamp of the most recent PPS edge capture in each clock domain
 * @param tsResult array of at least length 3 that will be populated with the
 *                 64-bit fixed-point format (32.32) timestamps
 *                 0 - monotonic clock
 *                 1 - frequency trimmed clock
 *                 2 - TAI clock
 */
void CLK_PPS(uint64_t *tsResult);

#endif //GPSDO_CLK_H
