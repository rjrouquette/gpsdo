
#include <msp430.h>
#include "math.h"

union u16 {
    uint8_t byte[2];
    uint16_t full;
};

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
    // loop over each dividend word separately
    #pragma GCC unroll 0
    for(int8_t i = 3; i >= 0; --i) {
        // loop over each bit in dividend word
        #pragma GCC unroll 0
        for(uint8_t j = 0; j < 16; ++j) {
            // shift quotient to make room for new bit
            quo <<= 1u;
            // save msb of head register as carry flag
            carry = ((union u32)head).word[1] >> 15u;
            // shift msb of remainder into 32-bit head register
            head <<= 1u;
            head |= ((union i64)rem).word[i] >> 15u;
            ((union i64)rem).word[i] <<= 1u;
            // compare head to divisor
            if(carry || head >= div) {
                head -= div;
                quo |= 1;
            }
        }
    }

    // sign restoration
    return neg ? -quo : quo;
}

// signed integer multiplication
int32_t mult16s16s(int16_t a, int16_t b) {
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
int32_t mult24s8s(int32_t a, int16_t b) {
    union i32 res;

    // use hardware multiplier
    MPYS32L = ((union i32)a).word[0];
    MPYS32H = ((union i32)a).word[1];
    OP2 = b;
    // get result
    res.word[0] = RESLO;
    res.word[1] = RESHI;
    return res.full;
}

// signed integer multiplication
int64_t mult32s32s(int32_t a, int32_t b) {
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

// integer square root
uint32_t sqrt64(uint64_t num) {
    uint64_t res = 0;
    uint64_t bit = 1ull << 62u;

    // find highest power of 4 less than num
    while (bit > num)
        bit >>= 2u;

    // digit-by-digit method
    while (bit > 0) {
        if (num >= res + bit) {
            num -= res + bit;
            res = (res >> 1u) + bit;
        } else
            res >>= 1u;
        bit >>= 2u;
    }
    return res;
}


// Hex string lookup tables
#define FLASH __attribute__ ((section(".text.const")))
// ASCII hex to integer lookup table
FLASH const uint8_t fromHex4[128] = {
    ['0'] = 0x0,
    ['1'] = 0x1,
    ['2'] = 0x2,
    ['3'] = 0x3,
    ['4'] = 0x4,
    ['5'] = 0x5,
    ['6'] = 0x6,
    ['7'] = 0x7,
    ['8'] = 0x8,
    ['9'] = 0x9,

    ['A'] = 0xA,
    ['B'] = 0xB,
    ['C'] = 0xC,
    ['D'] = 0xD,
    ['E'] = 0xE,
    ['F'] = 0xF,

    ['a'] = 0xA,
    ['b'] = 0xB,
    ['c'] = 0xC,
    ['d'] = 0xD,
    ['e'] = 0xE,
    ['f'] = 0xF
};

// hex conversion
uint8_t fromHex8(const char *hex) {
    uint8_t res = fromHex4[(uint8_t)hex[0]];
    res <<= 4u;
    return res | fromHex4[(uint8_t)hex[1]];
}

// hex conversion
uint16_t fromHex16(const char *hex) {
    union u16 res;
    res.byte[1] = fromHex8(hex + 0);
    res.byte[0] = fromHex8(hex + 2);
    return res.full;
}

// hex conversion
uint32_t fromHex32(const char *hex) {
    union u32 res;
    res.word[1] = fromHex16(hex + 0);
    res.word[0] = fromHex16(hex + 4);
    return res.full;
}
