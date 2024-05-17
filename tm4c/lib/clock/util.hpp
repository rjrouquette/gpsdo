//
// Created by robert on 4/26/23.
//

#pragma once

#include <cstdint>

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
 * Compute offset correction for frequency trimming
 * @param rate frequency trim rate (signed 0.31)
 * @param delta elapsed time since prior offset adjustment (signed 31.32)
 * @return offset adjustment (signed 31.32)
 */
int64_t corrValue(int32_t rate, int64_t delta);

/**
 * Compute offset correction for frequency trimming
 * @param rate frequency trim rate (signed 0.31)
 * @param delta elapsed time since prior offset adjustment (signed 0.31)
 * @param rem offset remainder from prior adjustment (unsigned 0.32)
 * @return offset adjustment (signed 0.31)
 */
int32_t corrFracRem(int32_t rate, int32_t delta, volatile uint32_t &rem);

/**
 * Compute offset correction for frequency trimming
 * @param rate frequency trim rate (signed 0.31)
 * @param delta elapsed time since prior offset adjustment (unsigned 0.32)
 * @param rem offset remainder from prior adjustment (unsigned 0.32)
 * @return offset adjustment (signed 0.31)
 */
inline int32_t corrFracRem(const int32_t rate, const uint32_t delta, volatile uint32_t &rem) {
    return corrFracRem(rate, static_cast<int32_t>(delta), rem);
}

/**
 * Compute offset correction for frequency trimming
 * @param rate frequency trim rate (signed 0.31)
 * @param delta elapsed time since prior offset adjustment (signed 31.32)
 * @param rem offset remainder from prior adjustment (unsigned 0.32)
 * @return offset adjustment (signed 0.31)
 */
inline int32_t corrFracRem(const int32_t rate, const int64_t delta, volatile uint32_t &rem) {
    return corrFracRem(rate, static_cast<int32_t>(delta), rem);
}

/**
 * Compute offset correction for frequency trimming
 * @param rate frequency trim rate (signed 0.31)
 * @param delta elapsed time since prior offset adjustment (unsigned 32.32)
 * @param rem offset remainder from prior adjustment (unsigned 0.32)
 * @return offset adjustment (signed 0.31)
 */
inline int32_t corrFracRem(const int32_t rate, const uint64_t delta, volatile uint32_t &rem) {
    return corrFracRem(rate, static_cast<uint32_t>(delta), rem);
}

/**
 * Convert 64-bit fixed-point timestamp (32.32) to float
 * @param value to convert
 * @return single-precision floating point value
 */
float toFloatU(uint64_t value);

/**
 * Convert signed 64-bit fixed-point timestamp (31.32) to float
 * @param value to convert
 * @return single-precision floating point value
 */
float toFloat(int64_t value);

/**
 * Convert floating number to signed 64-bit fixed point timestamp (31.32)
 * @param value to convert
 * @return signed 64-bit fixed point timestamp (31.32)
 */
int64_t toFixedPoint(float value);
