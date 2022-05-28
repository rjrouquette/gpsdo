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
#include "lib/gps.h"
#include "lib/temp.h"


#define OFFSET_COARSE_ALIGN (1000000) // 1 millisecond
#define STAT_TIME_CONST (16)
#define STAT_LOCK_RMS (250e-9f)


static int32_t ppsGpsEdge;
static int32_t ppsOutEdge;
static int ppsOffsetNano;

static float pllBias;

static float ppsOffsetMean;
static float ppsOffsetVar;
static float ppsOffsetRms;

static uint8_t ppsGpsReady;
static uint8_t ppsOutReady;
static uint8_t isGpsLocked;
static uint8_t waitRealign;


// temperature compensation
#define COMP_SPAN (8)
#define COMP_PREC (8)
#define COMP_MASK (0xFF)
static uint16_t compTau[256];
static float compMat[256];
static float compM, compB;

// current correction values
static float currFeedback;
static float currTemperature;
static float currCompensation;

static void setFeedback(float feedback);
static float getFeedback();
static void updateCompensation();
static float getCompensation();


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

    // use command mode
    EMAC0.PPSCTRL.TRGMODS0 = 3;
    // start 1 Hz PPS output
    EMAC0.PPSCTRL.PPSEN0 = 0;
    EMAC0.PPSCTRL.PPSCTRL = 1;
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

    // init GPS
    GPS_init();
}

void GPSDO_run() {
    // run GPS state machine
    GPS_run();

    // only update if GPS has lock
    if(!isGpsLocked) {
        ppsGpsReady = 0;
        ppsOutReady = 0;
        return;
    }

    // wait for both edge events
    if(!(ppsGpsReady && ppsOutReady))
        return;
    // clear ready state
    ppsGpsReady = 0;
    ppsOutReady = 0;
    // update current temperature
    currTemperature = ldexpf(TEMP_dcxo(), -8);

    // coarse realignment in progress
    if(waitRealign > 0) {
        --waitRealign;
        return;
    }

    // compute offset
    int offset = ppsOutEdge - ppsGpsEdge;
    // convert to nano seconds
    offset <<= 3;
    // reorder events if necessary
    if(offset < -500000000) { ppsOutReady = 1; return; }
    if(offset >  500000000) { ppsOutReady = 1; return; }
    // update PPS offset
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

    // update control loop
    float fltOffset = ((float) offset) * 1e-9f;
    currCompensation = getCompensation();
    setFeedback(currCompensation + pllBias + ldexpf(fltOffset, -4));
    pllBias += ldexpf(fltOffset, -8);

    // restrict offset
    if(fltOffset > 10e6f) fltOffset = 10e6f;
    if(fltOffset < -10e6f) fltOffset = -10e6f;
    // update PPS stats
    ppsOffsetMean += (fltOffset - ppsOffsetMean) / STAT_TIME_CONST;
    fltOffset *= fltOffset;
    ppsOffsetVar += (fltOffset - ppsOffsetVar) / STAT_TIME_CONST;
    ppsOffsetRms = sqrtf(ppsOffsetVar);

    // update temperature coefficient
    if(ppsOffsetRms < STAT_LOCK_RMS)
        updateCompensation();
}

int GPSDO_isLocked() {
    if(!isGpsLocked) return 0;
    return (ppsOffsetRms < STAT_LOCK_RMS);
}

int GPSDO_offsetNano() {
    return ppsOffsetNano;
}

float GPSDO_offsetMean() {
    return ppsOffsetMean;
}

float GPSDO_offsetRms() {
    return ppsOffsetRms;
}

float GPSDO_freqCorr() {
    return getFeedback();
}

float GPSDO_compBias() {
    return compB;
}

float GPSDO_compCoeff() {
    return compM;
}

float GPSDO_compValue() {
    return currCompensation;
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
    ppsOutReady ^= 1;
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
    ppsGpsReady ^= 1;
}

void setFeedback(float feedback) {
    // update current feedback value
    currFeedback = feedback;
    // convert to correction factor
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

void updateCompensation() {
    // update temperature bins
    const int ibase = (int) (currTemperature * COMP_PREC);
    volatile uint16_t *tau = &compTau[ibase & COMP_MASK];
    volatile float *step = &compMat[ibase & COMP_MASK];
    // update selected bin
    if(tau[0] < 4096) ++tau[0];
    step[0] += (currFeedback - step[0]) / (float) tau[0];
}

float getCompensation() {
    const int ibase = (int) (currTemperature * COMP_PREC);

    float m = 0;
    int cnt = 0;
    float scratch[COMP_SPAN * 2];
    for(int i = -COMP_SPAN; i <= (COMP_SPAN-1); i++) {
        float l = compMat[(ibase + i) & COMP_MASK];
        float u = compMat[(ibase + i + 1) & COMP_MASK];
        if(l != 0 && u != 0) {
            scratch[cnt++] = u - l;
        }
    }
    if(cnt != 0) {
        float mean = 0;
        for (int i = 0; i < cnt; i++)
            mean += scratch[i];
        mean /= (float) cnt;

        float stddev = 0;
        for (int i = 0; i < cnt; i++) {
            float diff = scratch[i] - mean;
            stddev += diff * diff;
        }
        stddev = sqrtf(stddev / (float) cnt);

        int n = 0;
        for (int i = 0; i < cnt; i++) {
            float diff = scratch[i] - mean;
            if (fabsf(diff) <= stddev) {
                m += scratch[i];
                ++n;
            }
        }
        if (n > 0) {
            m /= (float) n;
            m *= COMP_PREC;
        }
    }
    compM = m;

    const float b = compMat[ibase & COMP_MASK];
    if(b != 0) compB = b;
    const float o = (((float)ibase) + 0.5f) / COMP_PREC;
    return (compM * (currTemperature - o)) + compB;
}
