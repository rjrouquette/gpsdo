
#include "math.h"

// integer long-division
void divU(uint32_t div, uint64_t *rem, uint32_t *quo) {
    quo = 0;
    for(uint8_t i = 0; i < 64; i++) {
        (*quo) <<= 1;
        const uint8_t shift = 63 - i;
        uint32_t top = (*rem) >> shift;
        if(top >= div) {
            (*rem) -= ((uint64_t)div) << shift;
            (*quo) |= 1;
        }
    }
}

// signed integer long-division
void divS(uint32_t div, int64_t *rem, int32_t *quo) {
    (*quo) = 0;
    const int8_t neg = (*rem) < 0;
    if(neg) (*rem) = -(*rem);
    divU(div, (uint64_t *) rem, (uint32_t *) quo);
    if(neg) {
        (*rem) = -(*rem);
        (*quo) = -(*quo);
    }
}

// signed integer multiplication
int32_t mult32s(int16_t a, int16_t b) {
    // TODO use hardware acceleration
    return ((int32_t)a) * ((int32_t)b);
}

// signed integer multiplication
int64_t mult64s(int32_t a, int32_t b) {
    // TODO use hardware acceleration
    return ((int64_t)a) * ((int64_t)b);
}
