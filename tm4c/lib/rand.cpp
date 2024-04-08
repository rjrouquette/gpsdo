#include "rand.h"
#include "../hw/sys.h"

static uint32_t state;

void RAND_init() {
    // initialize random state
    for(int i = 0; i < 4; i++)
        state += UNIQUEID.WORD[i];
}

uint32_t RAND_next() {
    // Algorithm "xor" from p. 4 of Marsaglia, "Xorshift RNGs"
    uint32_t x = state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return state = x;
}
