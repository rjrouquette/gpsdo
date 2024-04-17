//
// Created by robert on 4/26/23.
//

#include "util.hpp"

#include "mono.hpp"


uint32_t nanosToFrac(uint32_t nanos) {
    // multiply by the integer portion of 4.294967296
    nanos <<= 2;
    // compute correction for the fractional portion of 4.294967296
    uint64_t correction = nanos;
    correction *= 0x12E0BE82;
    // assemble final result (round-to-nearest to minimize average error)
    nanos += static_cast<uint32_t>(correction) >> 31;
    nanos += correction >> 32;
    return nanos;
}

int64_t corrValue(const int32_t rate, int64_t delta) {
    const bool neg = delta < 0;
    if (neg)
        delta = -delta;

    // compute integral correction
    uint64_t scratch = static_cast<uint32_t>(delta >> 32);
    scratch *= rate;

    // compute fractional correction
    uint64_t frac = static_cast<uint32_t>(delta);
    frac *= rate;

    scratch += static_cast<int32_t>(frac >> 32);
    return static_cast<int64_t>(neg ? -scratch : scratch);
}

int32_t corrFracRem(const int32_t rate, const int32_t delta, volatile uint32_t &rem) {
    // compute rate-based delta adjustment
    int64_t scratch = delta;
    scratch *= rate;
    // add prior remainder
    scratch += rem;
    // update remainder
    rem = static_cast<uint32_t>(scratch);
    // return rate-based delta adjustment
    return static_cast<int32_t>(scratch >> 32);
}

float toFloatU(const uint64_t value) {
    return static_cast<float>(static_cast<uint32_t>(value >> 32)) +
           0x1p-32f * static_cast<float>(static_cast<uint32_t>(value));
}

float toFloat(const int64_t value) {
    return (value < 0) ? -toFloatU(-value) : toFloatU(value);
}
