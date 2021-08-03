/*
Software defined TCXO controller
*/

#ifndef MSP430_PPS
#define MSP430_PPS

#include <stdint.h>

/**
 * Initialize the PPS module
**/
void PPS_init();

/**
 * Update the PPS module internal state
**/
void PPS_poll();

/**
 * Reset the PPS ready indicator
**/
void PPS_clearReady();

/**
 * Reset the PPS ready indicator
**/
uint8_t PPS_isReady();

/**
 * Get the time delta between the generated PPS and GPS PPS leading edges.
 * Value is restricted to 16-bit range
 * LSB represents 5ns
 * @return the time offset between PPS leading edges
**/
int16_t PPS_getDelta();

#endif
