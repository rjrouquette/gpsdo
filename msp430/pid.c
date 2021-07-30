
#include "math.h"
#include "pid.h"

// control loop coefficients
int32_t D = 0;
int32_t P = 0;
int32_t I = 0;

// integrator
int32_t prevError = 0;
int64_t acc = 0;

/**
 * Gets the DCXO adjustment for the given temperature.
 * @param trackingError - DCXO tracking error (same resolution as DCXO frequency offset)
 * @return the DCXO digital frequency offset
**/
int32_t PID_update(int32_t trackingError) {
    // update integrator
    acc += trackingError;

    // compute loop output
    int64_t out;
    // derivative
    out = mult64s(D, trackingError - prevError);
    // proportional
    out += mult64s(P, trackingError);
    // integral
    out += I * acc;
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
