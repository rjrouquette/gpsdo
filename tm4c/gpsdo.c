//
// Created by robert on 5/24/22.
//

#include <math.h>
#include "gpsdo.h"
#include "hw/adc.h"
#include "hw/emac.h"
#include "hw/gpio.h"
#include "hw/timer.h"
#include "lib/delay.h"
#include "lib/gps.h"


#define OFFSET_COARSE_ALIGN (125000) // 1 millisecond
#define STAT_TIME_CONST (16)
#define STAT_LOCK_RMS (250e-9f)
#define STAT_COMP_RMS (200e-9f)
#define TCOMP_ALPHA (0x1p-16f)

// temperature compensation state
static volatile float currTemp;
static volatile float tcompCoeff, tcompOffset;


static int32_t ppsGpsEdge;
static int32_t ppsOutEdge;
static int32_t ppsOffsetNano;

static float pllBias;
static float pllCorr;

static float ppsOffsetMean;
static float ppsOffsetVar;
static float ppsOffsetRms;
static float ppsSkewVar;
static float ppsSkewRms;

static uint8_t ppsReady;

// current correction values
static float currFeedback;
static float currCompensation;

static void setFeedback(float feedback);
static void updateTempComp(float target);

// ISR for temperature measurement
void ISR_ADC0Sequence3(void) {
    // clear interrupt
    ADC0.ISC.IN3 = 1;
    // update temperature
    int32_t temp = ADC0.SS3.FIFO.DATA;
    temp -= 2441;
    float _temp = (float) temp;
    _temp *= -0.0604248047f;
    _temp -= currTemp;
    _temp *= 0x1p-8f;
    currTemp += _temp;
    // start next temperature measurement
    ADC0.PSSI.SS3 = 1;
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
    // indicate that PPS is ready
    ppsReady = 1;
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
}

void initTempComp() {
    // Enable ADC0
    RCGCADC.EN_ADC0 = 1;
    delay_cycles_4();

    // configure ADC0 for temperature measurement
    ADC0.CC.CLKDIV = 0;
    ADC0.CC.CS = ADC_CLK_MOSC;
    ADC0.SAC.AVG = 6;
    ADC0.EMUX.EM3 = ADC_SS_TRIG_SOFT;
    ADC0.SS3.CTL.IE0 = 1;
    ADC0.SS3.CTL.END0 = 1;
    ADC0.SS3.CTL.TS0 = 1;
    ADC0.SS3.TSH.TSH0 = ADC_TSH_256;
    ADC0.IM.MASK3 = 1;
    ADC0.ACTSS.ASEN3 = 1;
    // trigger temperature measurement
    ADC0.PSSI.SS3 = 1;
}

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
    // rising edge
    GPTM5.CTL.TAEVENT = 0;
    GPTM5.CTL.TBEVENT = 0;
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
    initTempComp();

    // init GPS
    GPS_init();
}

void GPSDO_run() {
    // update temperature compensation
    float newComp = tcompOffset + (tcompCoeff * GPSDO_temperature());
    // prevent large correction impulses
    if(fabsf(newComp - currCompensation) > 100e-9f)
        pllBias -= (newComp - currCompensation);
    currCompensation = newComp;
    setFeedback(currCompensation + pllCorr + pllBias);

    // run GPS state machine
    GPS_run();

    // wait for pps output
    if(!ppsReady) return;
    // wait for window to expire
    const int32_t now = ((int32_t) GPTM4.TAV.raw);
    if(now - ppsOutEdge < OFFSET_COARSE_ALIGN)
        return;
    // clear ready flag
    ppsReady = 0;

    // return if GPS not ready or more than 1 second stale
    if(now - ppsGpsEdge > 150000000)
        return;

    // compute offset
    int32_t offset = ppsOutEdge - ppsGpsEdge;
    // perform coarse realignment if offset is too large
    if(offset < -OFFSET_COARSE_ALIGN)
        return;
    if(offset > OFFSET_COARSE_ALIGN) {
        // limit slew rate (10ms)
        if(offset > 1250000)
            offset = 1250000;
        // truncate to 40 ns resolution
        offset /= 5;
        offset *= 40;
        // set update registers
        EMAC0.TIMNANOU.NEG = 1;
        EMAC0.TIMNANOU.VALUE = offset;
        EMAC0.TIMSECU = 0;
        // wait for hardware ready state
        while(EMAC0.TIMSTCTRL.TSUPDT);
        // start update
        EMAC0.TIMSTCTRL.TSUPDT = 1;
        return;
    }

    // convert to nano seconds
    offset <<= 3;
    // update PPS offset
    float ppsSkew = 1e-9f * (float) (offset - ppsOffsetNano);
    ppsOffsetNano = offset;

    // convert PPS offset to float
    float fltOffset = ((float) offset) * 1e-9f;
    // determine compensation rate
    float rate = ppsOffsetRms / 256e-9f;
    if(rate > 0.5f) rate = 0.5f;
    // update control loop
    pllCorr = fltOffset * rate;
    setFeedback(currCompensation + pllCorr + pllBias);
    pllBias += fltOffset * 0x1p-9f;

    // update PPS stats
    ppsOffsetMean += (fltOffset - ppsOffsetMean) / STAT_TIME_CONST;
    fltOffset *= fltOffset;
    ppsOffsetVar += (fltOffset - ppsOffsetVar) / STAT_TIME_CONST;
    ppsOffsetRms = sqrtf(ppsOffsetVar);
    // update skew stats
    ppsSkew *= ppsSkew;
    // restrict skew
    if(ppsSkew > 1e-8f) ppsSkew = 1e-8f;
    ppsSkewVar += (ppsSkew - ppsSkewVar) / STAT_TIME_CONST;
    ppsSkewRms = sqrtf(ppsSkewVar);

    // update temperature compensation
    if(ppsSkewRms < STAT_COMP_RMS)
        updateTempComp(currFeedback);
}

int GPSDO_isLocked() {
    if(!GPS_hasLock()) return 0;
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

float GPSDO_skewRms() {
    return ppsSkewRms;
}

float GPSDO_freqCorr() {
    return currFeedback;
}

float GPSDO_temperature() {
    return currTemp;
}

float GPSDO_compBias() {
    return tcompOffset;
}

float GPSDO_compCoeff() {
    return tcompCoeff;
}

float GPSDO_compValue() {
    return currCompensation;
}

float GPSDO_pllValue() {
    return pllBias + pllCorr;
}

static void setFeedback(float feedback) {
    // convert to correction factor
    int32_t correction = lroundf(feedback * 0x1p32f);
    // correction factor must always be negative
    if(correction > -1) correction = -1;
    // correction factor must not exceed 1 part-per-thousand
    if(correction < -(1 << 22)) correction = -(1 << 22);
    // apply correction update
    EMAC0.TIMADD = correction;
    EMAC0.TIMSTCTRL.ADDREGUP = 1;
    // update current feedback value
    currFeedback = (float)correction * 0x1p-32f;
}

static void updateTempComp(float target) {
    // simple bootstrap to improve stability
    if(tcompCoeff == 0 && tcompOffset == 0)
        tcompOffset = target;

    // compute error
    float temp = currTemp;
    float error = target - tcompOffset;
    error -= tcompCoeff * temp;
    error *= TCOMP_ALPHA;

    // update compensation state
    tcompCoeff += error * temp;
    tcompOffset += error;
}
