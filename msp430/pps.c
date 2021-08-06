
#include <msp430.h>
#include "gps.h"
#include "pps.h"

// These values are tuned for 25 MHz clock
#define PPS_TARGET_INCR (190)
#define PPS_OFFSET_INCR (48160)
// Coarse Lock offset tolerance (1 us)
#define CTOL (25)

union i32 {
    uint16_t word[2];
    int32_t full;
};

// internal state
uint16_t coarseLocked = 0;
// PPS ISR macro timer
uint16_t ppsMacroTime = 0;
// PPS generation triggers
uint16_t ppsTarget = 0;
uint16_t ppsOffset = 0;
// GPS PPS capture
uint16_t gpsTarget = 0;
uint16_t gpsOffset = 0;
uint16_t gpsReady = 0;
// PPS delta calulation
uint16_t deltaTarget = 0;

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

    // configure input pins
    P1DIR &= ~(BIT6 | BIT7);
    P1SEL |=  (BIT6 | BIT7);
    P1REN &= ~(BIT6 | BIT7);
    P1OUT &= ~(BIT6 | BIT7);

    // configure TD1 for PPS generation
    // No grouping, 16-bit, submodule clcok, no clk divisor, continuous mode
    TD1CTL0 = TDCLGRP_0 | CNTL__16 | TDSSEL__SMCLK| ID__1 | MC__CONTINOUS;
    // no clk divisor
    TD1CTL1 = TDIDEX__1;
    // capture mode on channel 1
    TD1CTL2 = TDCAPM1;
    // output compare mode w/ interrupt
    TD1CCTL0 = CLLD_0 | OUTMOD_5 | CCIE;
    // positive edge capture on CCI0B
    TD1CCTL1 = CM_1 | CCIS_1 | SCS | CLLD_0 | CAP | OUTMOD_0;
    // disable high-res
    TD0HCTL0 = 0;
    TD0HCTL1 = 0;

    // configure input pins
    P2DIR |=  BIT1;     // P2-1 is MSP PPS output
    P2DIR &= ~BIT2;     // P2-2 is GPS PPS input
    P2SEL |=  (BIT1 | BIT2);
    P2REN &= ~(BIT1 | BIT2);
    P2OUT &= ~(BIT1 | BIT2);
}

/**
 * Update the PPS module internal state
 * return PPS delta measurement (5ns resolution)
**/
int16_t PPS_poll() {
    int16_t ppsTimer;
    // service PPS generation
    _disable_interrupts();
    ppsTimer = ppsMacroTime - ppsTarget;
    if(ppsTimer <= 0) {
        _enable_interrupts();
        return PPS_IDLE;
    }
    ppsTarget += PPS_TARGET_INCR;
    ppsOffset += PPS_OFFSET_INCR;
    _enable_interrupts();

    // process GPS PPS event
    if(gpsReady) {
        // clear status flag
        gpsReady = 0;

        // get timestamp snapshot
        _disable_interrupts();
        int16_t diffT = ppsTarget;
        int16_t diffO = ppsOffset;
        diffT -= gpsTarget;
        diffO -= gpsOffset;
        _enable_interrupts();
        // adjust for polling mis-alignment
        if(diffT >= PPS_TARGET_INCR) {
            diffT -= PPS_TARGET_INCR;
            diffO -= PPS_OFFSET_INCR;
        }
        // verify lock tolerance
        if(diffT == -1 && (diffO >= -CTOL) && (diffO <= CTOL)) {
            // coarse lock satisfied
            coarseLocked = 1;
        } else {
            // coarse lock has failed
            coarseLocked = 0;
            // forcefully align PPS edge
            ppsTarget = gpsTarget + PPS_TARGET_INCR;
            ppsOffset = gpsOffset + PPS_OFFSET_INCR;
            // set PPS since next update will be the half-way mark
            TD1CCTL0 &= ~OUTMOD2;
        }
    }

    // check if PPS delta is ready
    if(!(CCIFG & TD0CCTL0 & TD0CCTL1))
        return PPS_IDLE;
    // clear interrupt flags
    TD0CCTL0 &= ~(COV | CCIFG);
    TD0CCTL1 &= ~(COV | CCIFG);
    // check if GPS has fix
    if(!GPS_hasFix()) return PPS_NOLOCK;
    // check if PPS has coarse lock
    if(!coarseLocked) return PPS_NOLOCK;
    // return PPS delta
    return TD0CL1 - TD0CL0;
}

// output compare on TD1-0
__interrupt_vec(TIMER0_D1_VECTOR)
void TIMER0_D1_ISR() {
    // increment macro counter
    if(++ppsMacroTime == ppsTarget) {
        TD1CL0 = ppsOffset;
        TD1CCTL0 ^= OUTMOD2;
    }
    // clear overflow if set
    TD1CCTL0 &= ~COV;

    // check for GPS PPS
    if(TD1CCTL1 & CCIFG) {
        // clear interrupt flag
        TD1CCTL1 &= ~(COV | CCIFG);
        // record event time
        gpsOffset = TD1CL1;
        gpsTarget = ppsMacroTime;
        gpsReady = 1;
    }
}
