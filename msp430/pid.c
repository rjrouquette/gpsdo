
#include <msp430.h>
#include "config.h"
#include "pid.h"

union i32 {
    uint16_t word[2];
    int32_t full;
};

union i64 {
    uint16_t word[4];
    int64_t full;
};

// control loop coefficients
int32_t D = 0;
int32_t P = 0;
int32_t I = 0;

// integrator
int32_t prevError = 0;
int32_t acc = 0;

/**
 * Initialize the PID controller
**/
void PID_init() {
    PID_setCoeff(sysConf.pidDeriv, sysConf.pidProp, sysConf.pidInteg);
}

/**
 * Gets the DCXO adjustment for the given temperature.
 * @param error - DCXO tracking error (same resolution as DCXO frequency offset)
 * @return the DCXO digital frequency offset
**/
int32_t PID_update(int32_t error) {
    // normal MAC operation w/ result protection
    MPY32CTL0 = MPYDLY32;

    // proportional term (HW MAC)
    MPYS32L = ((union i32)P).word[0];
    MPY32H = ((union i32)P).word[1];
    OP2L = ((union i32)error).word[0];
    OP2H = ((union i32)error).word[1];
    
    // compute error derivative (10 cycles)
    int32_t delta = error - prevError;

    // derivative term (HW MAC)
    MACS32L = ((union i32)D).word[0];
    MACS32H = ((union i32)D).word[1];
    OP2L = ((union i32)delta).word[0];
    OP2H = ((union i32)delta).word[1];

    // update derivative (8 cycles)
    prevError = error;

    // integral term (HW MAC)
    MACS32L = ((union i32)I).word[0];
    MACS32H = ((union i32)I).word[1];
    OP2L = ((union i32)acc).word[0];
    OP2H = ((union i32)acc).word[1];

    // update integrator (10 cycles)
    acc += error;

    // return result
    union i64 res;
    res.word[0] = RES0;
    res.word[1] = RES1;
    res.word[2] = RES2;
    res.word[3] = RES3;
    return res.full >> 24u;
}

/**
 * Set the PID coefficients to the given values
 * @param d - the derivative coefficient in 8.24 fixed point format
 * @param p - the proportional coefficient in 8.24 fixed point format
 * @param i - the integral coefficient in 8.24 fixed point format
**/
void PID_setCoeff(int32_t d, int32_t p, int32_t i) {
    // set coefficients
    D = d;
    P = p;
    I = i;
    // clear integrator
    acc = 0;
}

/**
 * Reset the PID accumulator
**/
void PID_clearIntegral() {
    acc = 0;
}
