//
// Created by robert on 5/24/22.
//

#include <stdlib.h>
#include <math.h>
#include "gpsdo.h"
#include "hw/emac.h"
#include "hw/gpio.h"
#include "hw/timer.h"
#include "lib/delay.h"


#define OFFSET_COARSE_ALIGN (1000000) // 1 millisecond


static int32_t ppsGpsEdge;
static int32_t ppsOutEdge;
static int ppsOffsetNano;

static uint8_t ppsGpsReady;
static uint8_t ppsOutReady;
static uint8_t isGpsLocked;
static uint8_t waitRealign;


void setFeedback(float feedback);
float getFeedback();


void initPPS() {
    // configure PPS output pin
    RCGCGPIO.EN_PORTG = 1;
    delay_cycles_4();
    // unlock GPIO config
    PORTG.LOCK = GPIO_LOCK_KEY;
    PORTG.CR = 0x01u;
    // configure pins
    PORTG.DIR = 0x01u;
    PORTG.DR8R = 0x01u;
    PORTG.PCTL.PMC0 = 0x5;
    PORTG.AFSEL.ALT0 = 1;
    PORTG.DEN = 0x01u;
    // lock GPIO config
    PORTG.CR = 0;
    PORTG.LOCK = 0;

    // 1 Hz 10% duty cycle
    EMAC0.PPS0INTVL = 25000000-1;
    EMAC0.PPS0WIDTH = 12500000-1;
    // use command mode
    EMAC0.PPSCTRL.TRGMODS0 = 3;
    // start 1 Hz PPS output
    EMAC0.PPSCTRL.PPSEN0 = 0;
    EMAC0.PPSCTRL.PPSCTRL = 1;
//    // start zero-aligned pulse train
//    EMAC0.PPSCTRL.PPSEN0 = 1;
//    EMAC0.TARGNANO = 0;
//    EMAC0.TARGSEC = EMAC0.TIMSEC + 1;
//    EMAC0.PPSCTRL.PPSCTRL = 2;
}

void initEdgeComp() {
    // Enable Timer 4
    RCGCTIMER.EN_GPTM4 = 1;
    delay_cycles_4();
    // Configure Timer 4 as 32-bit free-running timer
    GPTM4.TAILR = -1;
    GPTM4.TAMR.MR = 0x2;
    GPTM4.TAMR.CDIR = 1;
    GPTM4.TAMR.CINTD = 0;
    // start timer
    GPTM4.CTL.TAEN = 1;

    // Enable Timer 5
    RCGCTIMER.EN_GPTM5 = 1;
    delay_cycles_4();
    // Configure Timer 5 for capture mode
    GPTM5.CFG.GPTMCFG = 4;
    GPTM5.TAMR.MR = 0x3;
    GPTM5.TBMR.MR = 0x3;
    // edge-time mode
    GPTM5.TAMR.CMR = 1;
    GPTM5.TBMR.CMR = 1;
    GPTM5.CTL.TAEVENT = 1;
    GPTM5.CTL.TBEVENT = 1;
    // count up
    GPTM5.TAMR.CDIR = 1;
    GPTM5.TBMR.CDIR = 1;
    // disable overflow interrupt
    GPTM5.TAMR.CINTD = 0;
    GPTM5.TBMR.CINTD = 0;
    // full 24-bit count
    GPTM5.TAILR = 0xFFFF;
    GPTM5.TBILR = 0xFFFF;
    GPTM5.TAPR = 0xFF;
    GPTM5.TBPR = 0xFF;
    // interrupts
    GPTM5.IMR.CAE = 1;
    GPTM5.IMR.CBE = 1;
    GPTM0.TAMR.MIE = 1;
    GPTM0.TBMR.MIE = 1;
    // start timer
    GPTM5.CTL.TAEN = 1;
    GPTM5.CTL.TBEN = 1;
    // synchronize timers
    GPTM0.SYNC = (3 << 10);

    // configure capture pins
    RCGCGPIO.EN_PORTM = 1;
    delay_cycles_4();
    // unlock GPIO config
    PORTM.LOCK = GPIO_LOCK_KEY;
    PORTM.CR = 0xC0u;
    // configure pins;
    PORTM.PCTL.PMC6 = 3;
    PORTM.PCTL.PMC7 = 3;
    PORTM.AFSEL.ALT6 = 1;
    PORTM.AFSEL.ALT7 = 1;
    PORTM.DEN = 0xC0u;
    // lock GPIO config
    PORTM.CR = 0;
    PORTM.LOCK = 0;
}

void GPSDO_init() {
    initPPS();
    initEdgeComp();
    // TODO get this status from the GPS receiver
    isGpsLocked = 1;
    // wait at least 10 PPS updates before attempting lock
    waitRealign = 10;
}

void GPSDO_run() {
    // only update if GPS has lock
    if(!isGpsLocked) {
        ppsGpsReady = 0;
        ppsOutReady = 0;
        return;
    }

    // wait for both edge events
    if(!(ppsGpsReady && ppsOutReady))
        return;

    // coarse realignment in progress
    if(waitRealign) {
        --waitRealign;
        return;
    }

    // compute offset
    int offset = ppsOutEdge - ppsGpsEdge;
    // convert to nano seconds
    offset <<= 8;
    ppsOffsetNano = offset;

    // perform coarse realignment if offset is too large
    if(abs(offset) > OFFSET_COARSE_ALIGN) {
        // realign TAI second boundary
        waitRealign = 2;
        // truncate offset precision to 40ns
        offset %= 1000000000;
        offset /= 40;
        offset *= 40;
        // adjust sign
        if(offset > 0) {
            offset = -offset;
            EMAC0.TIMNANOU.NEG = 1;
        }
        // set update update registers
        EMAC0.TIMNANOU.VALUE = -offset;
        EMAC0.TIMSECU = 0;
        // wait for hardware ready state
        while(EMAC0.TIMSTCTRL.TSUPDT);
        // start update
        EMAC0.TIMSTCTRL.TSUPDT = 1;
        return;
    }

    // TODO implement PLL logic
    setFeedback(0.9e-9f * (float) offset);
}

int GPSDO_isLocked() {
    return (int) isGpsLocked;
}

int GPSDO_offsetNano() {
    // TODO replace this with EMA value
    return ppsOffsetNano;
}

float GPSDO_getCorrection() {
    return getFeedback();
}

// capture rising edge of output PPS for offset measurement
void ISR_Timer5A() {
    // compute edge time
    int32_t now = GPTM4.TAV.raw;
    uint16_t delta = GPTM5.TAV.LO;
    delta -= GPTM5.TAR.LO;
    ppsOutEdge = now - delta;
    // clear interrupt flag
    GPTM5.ICR.CAE = 1;
    // indicate variable is ready
    ppsOutReady = 1;
}

// capture rising edge of GPS PPS for offset measurement
void ISR_Timer5B() {
    // compute edge time
    int32_t now = GPTM4.TAV.raw;
    uint16_t delta = GPTM5.TBV.LO;
    delta -= GPTM5.TBR.LO;
    ppsGpsEdge = now - delta;
    // clear interrupt flag
    GPTM5.ICR.CBE = 1;
    // indicate variable is ready
    ppsGpsReady = 1;
}

void setFeedback(float feedback) {
    int32_t correction = lroundf(ldexpf(feedback, 32));
    // correction factor must always be negative
    if(correction > 0) correction = -1;
    // correction factor must not exceed 1 part-per-thousand
    if(correction < -(1 << 22)) correction = -(1 << 22);
    // apply correction update
    EMAC0.TIMADD = correction;
    EMAC0.TIMSTCTRL.ADDREGUP = 1;
}

float getFeedback() {
    return ldexpf((float)(int32_t)(EMAC0.TIMADD), -32);
}
