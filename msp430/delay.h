/*
Delay Function
*/
#ifndef MSP430_DELAY
#define MSP430_DELAY

#include <stdint.h>

/**
 * Delay by N loop iterations
 * @param n - number of loop iterations (approx 3 cpu cycles per iteration)
 */
inline void delayLoop(uint16_t n) {
    while(n--) asm ("");
}

#endif
