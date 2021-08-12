
#ifndef MSP430_INT_MATH
#define MSP430_INT_MATH

#include <stdint.h>

// signed/unsigned integer long-division
int32_t div64s32u(int64_t dividend, uint32_t divisor) __attribute__ ((const));

// signed integer multiplication
int32_t mult16s16s(int16_t a, int16_t b) __attribute__ ((const));
// signed integer multiplication
int32_t mult24s8s(int32_t a, int16_t b) __attribute__ ((const));
// signed integer multiplication
int64_t mult32s32s(int32_t a, int32_t b) __attribute__ ((const));

// integer square root
uint32_t sqrt64(uint64_t num) __attribute__ ((const));

// hex conversion
uint8_t fromHex8(const char *hex);
// hex conversion
uint16_t fromHex16(const char *hex);
// hex conversion
uint32_t fromHex32(const char *hex);

// hex conversion
void toHex8(char *hex, uint8_t value);
// hex conversion
void toHex16(char *hex, uint16_t value);
// hex conversion
void toHex32(char *hex, uint32_t value);

#endif
