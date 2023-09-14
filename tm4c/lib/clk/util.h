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
 * Convert nanoseconds to fraction
 * @param nanos 0 - 999999999
 * @return 32-bit fixed-point fractional timestamp (0.32)
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
 * @param rate frequency trim rate (signed 0.31)
 * @param delta elapsed time since prior offset adjustment (signed 31.32)
 * @return offset adjustment (signed 31.32)
 */
int64_t corrValue(int32_t rate, int64_t delta);

/**
 * Compute offset correction for frequency trimming
 * @param rate frequency trim rate (signed 0.31)
 * @param delta elapsed time since prior offset adjustment (unsigned 0.32)
 * @param rem offset remainder from prior adjustment (unsigned 0.32)
 * @return offset adjustment (signed 0.31)
 */
int32_t corrFrac(int32_t rate, uint32_t delta, volatile uint32_t *rem);

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
