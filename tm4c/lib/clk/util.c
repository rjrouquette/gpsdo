//
// Created by robert on 4/26/23.
//

#include "mono.h"
#include "util.h"

uint64_t fromClkMono(uint32_t timer, uint32_t offset, uint32_t integer) {
    int32_t ticks = (int32_t) (timer - offset);
    // adjust for underflow
    while(ticks < 0) {
        --integer;
        ticks += CLK_FREQ;
    }
    // adjust for overflow
    while(ticks >= CLK_FREQ) {
        ++integer;
        ticks -= CLK_FREQ;
    }

    // multiply by 32 (8ns per tick, and the integer portion of 4.294967296)
    ticks <<= 5;
    // compute correction value
    union fixed_32_32 scratch;
    scratch.ipart = 0;
    scratch.fpart = ticks;
    scratch.full *= 0x12E0BE82;
    // round up to minimize average error
    scratch.ipart += scratch.fpart >> 31;
    // combine correction value with base value
    scratch.fpart = ticks + scratch.ipart;
    // set seconds
    scratch.ipart = integer;
    return scratch.full;
}

int64_t corrValue(int32_t rate, int64_t delta) {
    union fixed_32_32 scratch;
    int neg = delta < 0;
    scratch.full = neg ? -delta : delta;

    // compute fractional correction
    uint32_t rem = 0;
    int32_t frac = corrFrac(rate, scratch.fpart, &rem);

    // compute integral correction
    scratch.fpart = 0;
    scratch.ipart = corrFrac(rate, scratch.ipart, &scratch.fpart);
    scratch.full += frac;

    if(neg) scratch.full = -scratch.full;
    return (int64_t) scratch.full;
}

int32_t corrFrac(int32_t rate, uint32_t delta, volatile uint32_t *rem) {
    union fixed_32_32 scratch;
    scratch.ipart = 0;
    scratch.fpart = delta;
    scratch.full *= rate;
    scratch.full += *rem;
    *rem = scratch.fpart;
    return (int32_t) scratch.ipart;
}

float toFloatU(uint64_t value) {
    union fixed_32_32 scratch;
    scratch.full = value;
    float temp = 0x1p-32f * (float) scratch.fpart;
    temp += (float) scratch.ipart;
    return temp;
}

float toFloat(int64_t value) {
    union fixed_32_32 scratch;
    scratch.full = value;
    float temp = 0x1p-32f * (float) scratch.fpart;
    temp += (float) (int32_t) scratch.ipart;
    return temp;
}
