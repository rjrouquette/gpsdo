//
// Created by robert on 4/26/23.
//

#include "mono.h"
#include "util.h"

uint32_t nanosToFrac(uint32_t nanos) {
    // multiply by 4 (integer portion of 4.294967296)
    nanos <<= 2;
    // compute correction value
    union fixed_32_32 scratch;
    scratch.ipart = 0;
    scratch.fpart = nanos;
    scratch.full *= 0x12E0BE82;
    // round up to minimize average error
    scratch.ipart += scratch.fpart >> 31;
    // combine correction value with base value
    return nanos + scratch.ipart;
}

uint64_t fromClkMono(uint32_t timer, uint32_t offset, uint32_t integer) {
    int32_t ticks = (int32_t) (timer - offset);
    // adjust for underflow
    while(ticks <= CLK_FREQ) {
        --integer;
        ticks += CLK_FREQ;
    }
    // adjust for overflow
    while(ticks >= CLK_FREQ) {
        ++integer;
        ticks -= CLK_FREQ;
    }

    // assemble result
    union fixed_32_32 result;
    result.fpart = nanosToFrac(ticks << 3);
    result.ipart = integer;
    return result.full;
}

uint64_t corrValue(uint32_t rate, uint64_t delta, uint32_t *rem) {
    int neg = 0;
    if(delta >> 63) {
        delta = -delta;
        neg = 1;
    }
    if(rate >> 31 & 1) {
        rate = -rate;
        neg ^= 1;
    }
    delta *= rate;
    if(neg) delta = -delta;
    if(rem) {
        delta += *rem;
        *rem = (uint32_t) delta;
    }
    return ((int64_t) delta) >> 32;
}
