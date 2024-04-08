//
// Created by robert on 3/27/24.
//

#include "util.hpp"
#include "../clk/util.hpp"
#include "../net/util.h"

#include <memory.h>


// convert 64-bit fixed point timestamp to chrony TimeSpec
void chrony::toTimespec(const uint64_t timestamp, volatile Timespec *ts) {
    fixed_32_32 scratch = {};
    scratch.full = timestamp;
    // rough reduction from fraction to nanoseconds
    const uint32_t temp = scratch.fpart;
    scratch.fpart -= temp >> 4;
    scratch.fpart -= temp >> 7;
    ts->tv_nsec = htonl(scratch.fpart >> 2);
    // set integer seconds
    ts->tv_sec_low = htonl(scratch.ipart);
    ts->tv_sec_high = 0;
}

// convert IEEE 754 float to candm.h style float
int32_t chrony::htonf(const float value) {
    // decompose IEEE 754 single-precision float
    int32_t raw;
    memcpy(&raw, &value, sizeof(raw));
    uint32_t coef = raw & ((1 << 23) - 1);
    const int32_t exp = ((raw >> 23) & 0xFF) - 127;

    // small values and NaNs
    if (exp < -65 || (exp == 128 && coef != 0))
        return 0;

    // large values
    if (exp > 61)
        return static_cast<int32_t>(htonl((raw < 0) ? 0xFF000000u : 0xFEFFFFFFu));

    // normal values
    coef |= 1 << 23;
    if (raw < 0)
        coef = -coef;
    coef &= (1 << 25) - 1;
    coef |= (exp + 2) << 25;
    return static_cast<int32_t>(htonl(coef));
}
