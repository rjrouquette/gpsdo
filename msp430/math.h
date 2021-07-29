
#ifndef MSP430_INT_MATH
#define MSP430_INT_MATH

#include <stdint.h>

// integer long-division
void divU(uint32_t div, uint64_t *rem, uint32_t *quo);
// signed integer long-division
void divS(uint32_t div, int64_t *rem, int32_t *quo);

// signed integer multiplication
int32_t mult32s(int16_t a, int16_t b);
// signed integer multiplication
int64_t mult64s(int32_t a, int32_t b);

#endif
