//
// Created by robert on 4/26/23.
//

#ifndef GPSDO_CLK_UTIL_H
#define GPSDO_CLK_UTIL_H

#include <stdint.h>

/**
 * fixed-point 32.32 timestamp structure
 */
union fixed_32_32 {
    struct {
        uint32_t fpart;
        uint32_t ipart;
    };
    uint64_t full;
};

/**
 * Convert nanoseconds into fractional second
 * @param nanos nanosecond value to convert
 * @return 32-bit fixed-point format (0.32)
 */
uint32_t nanosToFrac(uint32_t nanos);

/**
 * Convert monotonic clock components into timestamp
 * @param timer raw timer value (125 MHz)
 * @param offset most recent second boundary timer value
 * @param integer current integer count of seconds
 * return 64-bit fixed-point timestamp (32.32)
 */
uint64_t fromClkMono(uint32_t timer, uint32_t offset, uint32_t integer);

/**
 * Compute offset correction for frequency trimming
 * @param rate frequency trim rate (0.31)
 * @param delta elapsed time since prior offset adjustment (32.32)
 * @param rem offset remainder from prior adjustment (0.32)
 * @return offset adjustment (32.32)
 */
uint64_t corrValue(uint32_t rate, uint64_t delta, uint32_t *rem);

#endif //GPSDO_CLK_UTIL_H
