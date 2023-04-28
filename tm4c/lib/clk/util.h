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

/**
 * Convert 64-bit fixed-point timestamp (32.32) to float
 * @param value to convert
 * @return single-precision floating point value
 */
float toFloatU(uint64_t value);

/**
 * Convert signed 64-bit fixed-point timestamp (32.32) to float
 * @param value to convert
 * @return single-precision floating point value
 */
float toFloat(int64_t value);

#endif //GPSDO_CLK_UTIL_H
