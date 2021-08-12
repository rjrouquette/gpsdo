
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

// integrator
int32_t prevError = 0;
int32_t acc = 0;

/**
 * Initialize the PID controller
**/
void PID_init() {
    acc = 0;
    prevError = 0;
}

/**
 * Updates the GPS PID tracking loop.
 * @param error - DCXO tracking error (same resolution as DCXO frequency offset)
 * @return the DCXO digital frequency offset
**/
int32_t PID_update(int32_t error) {
    // normal MAC operation w/ result protection
    MPY32CTL0 = MPYDLY32;

    // proportional term (HW MAC)
    MPYS32L = ((union i32)sysConf.pid.P).word[0];
    MPYS32H = ((union i32)sysConf.pid.P).word[1];
    OP2L = ((union i32)error).word[0];
    OP2H = ((union i32)error).word[1];
    
    // compute error derivative (10 cycles)
    int32_t delta = error - prevError;

    // derivative term (HW MAC)
    MACS32L = ((union i32)sysConf.pid.D).word[0];
    MACS32H = ((union i32)sysConf.pid.D).word[1];
    OP2L = ((union i32)delta).word[0];
    OP2H = ((union i32)delta).word[1];

    // update derivative (8 cycles)
    prevError = error;

    // integral term (HW MAC)
    MACS32L = ((union i32)sysConf.pid.I).word[0];
    MACS32H = ((union i32)sysConf.pid.I).word[1];
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
 * @param p - the proportional coefficient in 8.24 fixed point format
 * @param i - the integral coefficient in 8.24 fixed point format
 * @param d - the derivative coefficient in 8.24 fixed point format
**/
void PID_setCoeff(int32_t p, int32_t i, int32_t d) {
    // set coefficients
    sysConf.pid.P = p;
    sysConf.pid.I = i;
    sysConf.pid.D = d;
    // clear integrator
    PID_clearIntegral();
}

/**
 * Reset the PID accumulator
**/
void PID_clearIntegral() {
    acc = 0;
}
