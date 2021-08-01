
#include <msp430.h>
#include "pps.h"


/**
 * Initialize the PPS module
**/
void PPS_init() {
    // configure TD0 for hi-res input capture
    // No grouping, 16-bit, submodule clcok, no clk divisor, continuous mode
    TD0CTL0 = TDCLGRP_0 | CNTL__16 | TDSSEL__SMCLK| ID__1 | MC__CONTINOUS;
    // no clk divisor, high-res mode
    TD0CTL1 = TDIDEX__1 | TDCLKM__HIGHRES;
    // capture mode on channels 0 and 1
    TD0CTL2 = TDCAPM0 | TDCAPM1;
    // positive edge capture on CCI0A
    TD0CCTL0 = CM_1 | CCIS_0 | SCS | CLLD_0 | CAP | OUTMOD_0;
    // positive edge capture on CCI0B
    TD0CCTL1 = CM_1 | CCIS_1 | SCS | CLLD_0 | CAP | OUTMOD_0;
    // no divider, 8x muliplier, high-res clk on, enhanced accuracy, regulation, high-res enabled
    TD0HCTL0 = TDHD_0 | TDHM__8 | TDHRON | TDHEAEN | TDHREGEN | TDHEN;
    // input clock is >15MHz
    TD0HCTL1 = TDHCLKCR;
}

/**
 * Reset the PPS ready indicator
**/
void PPS_clearReady() {
    TD0CCTL0 &= ~CCIFG;
    TD0CCTL1 &= ~CCIFG;
}

/**
 * Reset the PPS ready indicator
**/
uint8_t PPS_isReady() {
    return (TD0CCTL0 & CCIFG) | (TD0CCTL0 & CCIFG);
}

/**
 * Get the time delta between the generated PPS and GPS PPS leading edges.
 * Value is restricted to 16-bit range
 * LSB represents 5ns
 * @return the time offset between PPS leading edges
**/
int16_t PPS_getDelta() {
    return 0;
}
