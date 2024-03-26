//
// Created by robert on 4/26/23.
//

#include "mono.h"
#include "util.h"


__attribute__((optimize(3)))
uint32_t nanosToFrac(uint32_t nanos) {
    // multiply by the integer portion of 4.294967296
    nanos <<= 2;
    // compute correction for the fractional portion of 4.294967296
    uint64_t correction = nanos;
    correction *= 0x12E0BE82;
    // assemble final result (round-to-nearest to minimize average error)
    nanos += ((uint32_t) correction) >> 31;
    nanos += correction >> 32;
    return nanos;
}

uint64_t fromClkMono(const uint32_t timer, const uint32_t offset, uint32_t integer) {
    int32_t ticks = (int32_t) (timer - offset);
    // adjust for underflow
    while (ticks < 0) {
        ticks += CLK_FREQ;
        --integer;
    }
    // adjust for overflow
    while (ticks >= CLK_FREQ) {
        ticks -= CLK_FREQ;
        ++integer;
    }

    // assemble result
    union fixed_32_32 scratch;
    scratch.fpart = nanosToFrac(ticks * CLK_NANO);
    scratch.ipart = integer;
    return scratch.full;
}

__attribute__((optimize(3)))
int64_t corrValue(const int32_t rate, int64_t delta) {
    const int neg = delta < 0;
    if (neg)
        delta = -delta;

    // compute integral correction
    uint64_t scratch = (uint32_t) (delta >> 32);
    scratch *= rate;

    // compute fractional correction
    uint64_t frac = (uint32_t) delta;
    frac *= rate;

    scratch += (int32_t) (frac >> 32);
    return (int64_t) (neg ? -scratch : scratch);
}

__attribute__((optimize(3)))
int32_t corrFracRem(const int32_t rate, const int32_t delta, volatile uint32_t *rem) {
    // compute rate-based delta adjustment
    int64_t scratch = delta;
    scratch *= rate;
    // add prior remainder
    scratch += *rem;
    // update remainder
    *rem = (uint32_t) scratch;
    // return rate-based delta adjustment
    return (int32_t) (scratch >> 32);
}

__attribute__((optimize(3)))
float toFloatU(const uint64_t value) {
    return ((float) (uint32_t) (value >> 32)) + (0x1p-32f * (float) (uint32_t) value);
}

float toFloat(const int64_t value) {
    return (value < 0) ? -toFloatU(-value) : toFloatU(value);
}
