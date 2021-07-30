
#include <msp430.h>
#include "math.h"

union i32 {
    uint16_t word[2];
    int32_t full;
};

union u32 {
    uint16_t word[2];
    uint32_t full;
};

union i64 {
    uint16_t word[4];
    int64_t full;
};

// signed/unsigned integer long-division
int32_t div64s32u(int64_t rem, uint32_t div) {
    // sign detection
    const uint8_t neg = rem < 0;
    if(neg) rem = -rem;

    // unsigned long division
    uint8_t carry = 0;
    uint32_t head = 0;
    uint32_t quo = 0;
    for(int8_t shift = 63; shift >= 0; --shift) {
        // shift quotient to make room for new bit
        quo <<= 1u;
        // save msb of head register as carry flag
        carry = ((union u32)head).word[1] >> 15u;
        // shift msb of remainder into 32-bit head register
        head <<= 1u;
        head |= ((union i64)rem).word[3] >> 15u;
        rem <<= 1u;
        // compare head to divisor
        if(carry || head >= div) {
            head -= div;
            quo |= 1;
        }
    }

    // sign restoration
    return neg ? -quo : quo;
}

// signed integer multiplication
int32_t mult32s(int16_t a, int16_t b) {
    union i32 res;

    // use hardware multiplier
    MPYS = a;
    OP2 = b;
    // get result
    res.word[0] = RESLO;
    res.word[1] = RESHI;
    return res.full;
}

// signed integer multiplication
int64_t mult64s(int32_t a, int32_t b) {
    union i64 res;

    // use hardware multiplier
    MPYS32L = ((union i32)a).word[0];
    MPYS32H = ((union i32)a).word[1];
    OP2L = ((union i32)b).word[0];
    OP2H = ((union i32)b).word[1];
    // get result
    res.word[0] = RES0;
    res.word[1] = RES1;
    res.word[2] = RES2;
    res.word[3] = RES3;
    return res.full;
}
