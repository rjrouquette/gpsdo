/*
Software defined TCXO controller
*/

#ifndef MSP430_PPS
#define MSP430_PPS

#include <stdint.h>

#define PPS_IDLE (0x8000)
#define PPS_NOLOCK (0x8001)

/**
 * Initialize the PPS module
**/
void PPS_init();

/**
 * Update the PPS module internal state
 * return the time offset between PPS leading edges
**/
int16_t PPS_poll();

#endif
