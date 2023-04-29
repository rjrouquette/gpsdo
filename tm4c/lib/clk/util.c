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

uint64_t corrValue(uint32_t rate, uint64_t delta, uint32_t *rem) {
    int neg = 0;
    if(delta >> 63) {
        delta = -delta;
        neg = 1;
    }
    if(rate >> 31) {
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

float toFloatU(uint64_t value) {
    union fixed_32_32 scratch;
    scratch.full = value;
    float temp = 0x1p-32f * (float) scratch.fpart;
    temp += (float) scratch.ipart;
    return temp;
}

float toFloat(int64_t value) {
    if(value < 0)
        return -toFloatU((uint64_t) -value);
    else
        return toFloatU((uint64_t) value);
}
