//
// Created by robert on 5/24/22.
//

#include <math.h>
#include "gpsdo.h"
#include "hw/adc.h"
#include "hw/eeprom.h"
#include "hw/gpio.h"
#include "hw/interrupts.h"
#include "hw/timer.h"
#include "lib/clk.h"
#include "lib/delay.h"
#include "lib/gps.h"


#define CLK_FREQ (125000000) // 125 MHz
#define PPS_GRACE_PERIOD (125000) // 1 ms
#define PPS_COARSE_ALIGN (62500) // 500 us
#define PLL_RATE_MAX (0xfp-4f) // 0.9375
#define PLL_RATE_REF (256e-9f) // 256 ns RMSE
#define PLL_RATE_INT (0x1p-6f) // integration rate
#define PLL_PPS_STABLE ((ppsPresent & 0xFFFF) == 0xFFFF)
#define STAT_ALPHA (0x1p-4f)
#define STAT_LOCK_RMS (250e-9f)
#define STAT_COMP_RMS (500e-9f)
#define TCOMP_ALPHA (0x1p-14f)
#define TCOMP_EEPROM_BLOCK (0x0010)
#define TCOMP_SAVE_INTV (3600)
#define TCOMP_STARTUP (16)
#define NTP_MAX_SKEW (5e-6f)
#define NTP_RATE (0x1p6f)
#define NTP_OFFSET_CORR (0x1p-10f)
#define NTP_OFFSET_BIAS (0x1p-16f)

// temperature compensation state
static float currTemp;
static float tcompCoeff, tcompBias, tcompOffset;
static uint32_t tcompSaved;

static int32_t ppsGpsEdgePrev;
static int32_t ppsGpsEdge;
static int32_t ppsOutEdgePrev;
static int32_t ppsOutEdge;
static int32_t ppsOffsetNano;

static float pllBias;
static float pllCorr;

static float ppsOffsetMean;
static float ppsOffsetVar;
static float ppsOffsetRms;
static float ppsSkewVar;
static float ppsSkewRms;

static uint32_t ppsPresent;

// current correction values
static uint64_t timeTrimmed;
static float currFeedback;
static float currCompensation;

static inline void setFeedback(float feedback) {
    currFeedback = CLK_TAI_trim(feedback);
}

static void updateTempComp(float rate, float target);

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
    // snapshot edge time
    uint32_t a = GPTM0.TAV.raw;
    uint32_t b = GPTM5.TAR.raw;
    // compute edge time
    ppsOutEdge = (int32_t) (a - ((a - b) & 0xFFFF));
    // clear interrupt flag
    GPTM5.ICR.CAE = 1;
}

// capture rising edge of GPS PPS for offset measurement
void ISR_Timer5B() {
    // snapshot edge time
    uint32_t a = GPTM0.TAV.raw;
    uint32_t b = GPTM5.TBR.raw;
    // compute edge time
    ppsGpsEdge = (int32_t) (a - ((a - b) & 0xFFFF));
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

    // load temperature compensation parameters
    uint32_t word;
    EEPROM_seek(TCOMP_EEPROM_BLOCK);
    word = EEPROM_read();
    if(word != -1u) {
        // set temperature coefficient
        tcompCoeff = *(float *) &word;
        // load temperature bias
        word = EEPROM_read();
        tcompBias = *(float *) &word;
        // load correction offset
        word = EEPROM_read();
        tcompOffset = *(float *) &word;
    }
    // reset if invalid
    if(!isfinite(tcompCoeff) || !isfinite(tcompBias) || !isfinite(tcompOffset)) {
        tcompCoeff = 0;
        tcompBias = 0;
        tcompOffset = 0;
    }
}

void initEdgeComp() {
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
    // full count range
    GPTM5.TAILR = -1;
    GPTM5.TBILR = -1;
    // interrupts
    GPTM5.IMR.CAE = 1;
    GPTM5.IMR.CBE = 1;
    // start timer
    GPTM5.CTL.TAEN = 1;
    GPTM5.CTL.TBEN = 1;
    // synchronize with monotonic clock
    GPTM0.SYNC = 0x0c03;

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
}

void GPSDO_init() {
    initEdgeComp();
    initTempComp();

    // init GPS
    GPS_init();
}

void GPSDO_run() {
    // initialize temperature compensation
    if(CLK_MONOTONIC_INT() < TCOMP_STARTUP) {
        float newComp = currTemp;
        newComp -= tcompBias;
        newComp *= tcompCoeff;
        newComp += tcompOffset;
        currCompensation = newComp;
        setFeedback(currCompensation);
        return;
    }

    // update temperature compensation
    float newComp = currTemp;
    newComp -= tcompBias;
    newComp *= tcompCoeff;
    newComp += tcompOffset;
    // prevent large correction impulses
    if(fabsf(newComp - currCompensation) > 100e-9f)
        pllBias -= (newComp - currCompensation);
    currCompensation = newComp;
    setFeedback(currCompensation + pllCorr + pllBias);

    // wait for window to expire
    const int32_t now = (int32_t) GPTM0.TAV.raw;
    if((now - ppsOutEdge) < PPS_GRACE_PERIOD)
        return;
    // sanity check pps output interval
    int32_t ppsOutDelta = ppsOutEdge - ppsOutEdgePrev;
    ppsOutEdgePrev = ppsOutEdge;
    if(ppsOutDelta < CLK_FREQ - PPS_GRACE_PERIOD) return;
    if(ppsOutDelta > CLK_FREQ + PPS_GRACE_PERIOD) return;

    // advance PPS tracker
    ppsPresent <<= 1;
    // sanity check pps output interval
    int32_t ppsGpsDelta = ppsGpsEdge - ppsGpsEdgePrev;
    ppsGpsEdgePrev = ppsGpsEdge;
    // ignore if PPS event is unstable
    if(ppsGpsDelta < CLK_FREQ - PPS_GRACE_PERIOD) return;
    if(ppsGpsDelta > CLK_FREQ + PPS_GRACE_PERIOD) return;
    // mark PPS as present
    ppsPresent |= 1;

    // compute offset
    int32_t offset = ppsOutEdge - ppsGpsEdge;
    // perform coarse realignment if offset is too large
    if(
            offset >  PPS_COARSE_ALIGN ||
            offset < -PPS_COARSE_ALIGN
    ) {
        // clamp offset to +/- 0.5 seconds
        if(offset > (CLK_FREQ / 2)) offset -= CLK_FREQ;
        // convert to nanoseconds
        offset <<= 3;
        // trim TAI alignment
        CLK_TAI_align(offset);
        // record current offset
        ppsOffsetNano = offset;
        return;
    }

    // set update timestamp
    timeTrimmed = CLK_TAI();

    // convert to nano seconds
    offset <<= 3;
    // update PPS offset
    float ppsSkew = 1e-9f * (float) (offset - ppsOffsetNano);
    ppsOffsetNano = offset;

    // convert PPS offset to float
    float fltOffset = ((float) offset) * 1e-9f;
    // update PPS stats
    ppsOffsetMean += (fltOffset - ppsOffsetMean) * STAT_ALPHA;
    ppsOffsetVar += ((fltOffset * fltOffset) - ppsOffsetVar) * STAT_ALPHA;
    ppsOffsetRms = sqrtf(ppsOffsetVar);
    // update skew stats
    ppsSkew *= ppsSkew;
    // restrict skew
    if(ppsSkew > 1e-8f) ppsSkew = 1e-8f;
    ppsSkewVar += (ppsSkew - ppsSkewVar) * STAT_ALPHA;
    ppsSkewRms = sqrtf(ppsSkewVar);

    // determine compensation rate
    float rate = ppsOffsetRms / PLL_RATE_REF;
    if(rate > PLL_RATE_MAX) rate = PLL_RATE_MAX;
    // update control loop
    pllCorr = fltOffset * rate;
    if(PLL_PPS_STABLE)
        pllBias += pllCorr * PLL_RATE_INT;
    setFeedback(currCompensation + pllCorr + pllBias);

    // update temperature compensation
    if(ppsSkewRms < STAT_COMP_RMS)
        updateTempComp(1.0f, currFeedback);
}

int GPSDO_ntpUpdate(float offset, float skew) {
    if(ppsPresent) return 0;

    // set update timestamp
    timeTrimmed = CLK_TAI();

    // hard adjustment
    if(fabsf(offset) > 50e-3f) {
        // trim TAI alignment
        CLK_TAI_align((int32_t) roundf(offset * 1e9f));
        return 1;
    }

    // soft adjustment
    pllCorr = offset * NTP_OFFSET_CORR;
    pllBias += offset * NTP_OFFSET_BIAS;
    // update feedback
    setFeedback(currCompensation + pllCorr + pllBias);
    // update temperature compensation
    if(skew < NTP_MAX_SKEW && skew > 0)
        updateTempComp(NTP_RATE, currFeedback);
    return 0;
}

uint32_t GPSDO_ppsPresent() {
    return ppsPresent;
}

int GPSDO_isLocked() {
    if(!ppsPresent) return 0;
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
    return tcompBias;
}

float GPSDO_compOffset() {
    return tcompOffset;
}

float GPSDO_compCoeff() {
    return tcompCoeff;
}

float GPSDO_compValue() {
    return currCompensation;
}

uint32_t GPSDO_compSaved() {
    return tcompSaved;
}

float GPSDO_pllTrim() {
    return pllBias + pllCorr;
}

uint64_t GPSDO_updated() {
    return timeTrimmed;
}

static void updateTempComp(float rate, float target) {
    if(tcompBias == 0 && tcompOffset == 0) {
        tcompBias = currTemp;
        tcompOffset = target;
        return;
    }

    rate *= TCOMP_ALPHA;
    const float temp = currTemp;
    tcompBias += (temp - tcompBias) * rate;
    tcompOffset += (target - tcompOffset) * rate;

    float error = target;
    error -= tcompOffset;
    error -= tcompCoeff * (temp - tcompBias);
    error *= rate;
    error *= temp - tcompBias;
    tcompCoeff += error;

    const uint32_t now = CLK_MONOTONIC_INT();
    if(now - tcompSaved > TCOMP_SAVE_INTV) {
        tcompSaved = now;
        // save temperature compensation parameters
        EEPROM_seek(TCOMP_EEPROM_BLOCK);
        EEPROM_write(*(uint32_t *) &tcompCoeff);
        EEPROM_write(*(uint32_t *) &tcompBias);
        EEPROM_write(*(uint32_t *) &tcompOffset);
    }
}
