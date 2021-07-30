
#ifndef MSP430_INT_MATH
#define MSP430_INT_MATH

#include <stdint.h>

// signed/unsigned integer long-division
int32_t div64s32u32s(int64_t dividend, uint32_t divisor) __attribute__ ((const));

// signed integer multiplication
int32_t mult32s(int16_t a, int16_t b) __attribute__ ((const));
// signed integer multiplication
int64_t mult64s(int32_t a, int32_t b) __attribute__ ((const));

#endif
