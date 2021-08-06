/*
Software defined TCXO controller
*/

#ifndef MSP430_PID
#define MSP430_PID

#include <stdint.h>

/**
 * Initialize the PID controller
**/
void PID_init();

/**
 * Gets the DCXO adjustment for the given temperature.
 * @param error - DCXO tracking error (same resolution as DCXO frequency offset)
 * @return the DCXO digital frequency offset
**/
int32_t PID_update(int32_t error);

/**
 * Set the PID coefficients to the given values
 * @param d - the derivative coefficient in 8.24 fixed point format
 * @param p - the proportional coefficient in 8.24 fixed point format
 * @param i - the integral coefficient in 8.24 fixed point format
**/
void PID_setCoeff(int32_t d, int32_t p, int32_t i);

/**
 * Reset the PID accumulator
**/
void PID_clearIntegral();

#endif
