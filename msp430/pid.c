
#include "math.h"
#include "pid.h"

// control loop coefficients
int32_t D = 0;
int32_t P = 0;
int32_t I = 0;

// integrator
int32_t prevError = 0;
int32_t acc = 0;

/**
 * Gets the DCXO adjustment for the given temperature.
 * @param trackingError - DCXO tracking error (same resolution as DCXO frequency offset)
 * @return the DCXO digital frequency offset
**/
int32_t PID_update(int32_t trackingError) {
    // compute loop output
    int64_t out = acc;
    // derivative
    out = mult32s32s(D, trackingError - prevError);
    // proportional
    out += mult32s32s(P, trackingError);
    // integral
    out += mult32s32s(I, acc);
    // update integrator
    acc += trackingError;
    // return result
    return out >> 24u;
}

/**
 * Set the PID coefficients to the given values
 * @param d - the derivative coefficient in 8.24 fixed point format
 * @param p - the proportional coefficient in 8.24 fixed point format
 * @param i - the integral coefficient in 8.24 fixed point format
**/
void PID_setCoeff(int32_t d, int32_t p, int32_t i) {
    D = d;
    P = p;
    I = i;
}

/**
 * Reset the PID accumulator
**/
void PID_clearIntegral() {
    acc = 0;
}
