//
// Created by robert on 4/29/23.
//

#include "../clk/comp.h"
#include "pll.h"
#include "../clk/tai.h"

// PLL proportional terms
volatile int32_t offsetProportion;

// PLL integral terms
volatile int32_t offsetIntegral;
volatile int32_t driftIntegral;

// from ntp.c
void ntpApplyOffset(int64_t offset);

static void ntpSetTaiClock(int64_t offset) {
    CLK_TAI_adjust(offset);
    ntpApplyOffset(offset);
}

void PLL_updateOffset(int interval, int64_t offset, float rmsOffset) {
    // apply hard correction to TAI clock for large offsets
    if(offset > PLL_OFFSET_HARD_ALIGN) {
        ntpSetTaiClock(offset);
        return;
    }

    // restrict interval sign
    if(interval < 0) interval = 0;
    // compute proportional rate
    float rate = rmsOffset / PLL_OFFSET_CORR_BASIS;
    // limit proportional rate
    if(rate > PLL_OFFSET_CORR_MAX) rate = PLL_OFFSET_CORR_MAX;
    // compensate rate for polling interval
    rate *= 0x1p-31f * (float) (1 << (31 - interval));
    // update offset compensation
    float pllCorr = rate * -0x1p-32f * (float) (int32_t) offset;
    offsetProportion = (int32_t) (0x1p32f * pllCorr);
    if(rmsOffset < PLL_OFFSET_CORR_BASIS)
        offsetIntegral += offsetProportion >> PLL_OFFSET_INT_RATE;
    // limit integration range
    if(offsetIntegral > PLL_MAX_FREQ_TRIM) driftIntegral = PLL_MAX_FREQ_TRIM;
    if(offsetIntegral < PLL_MIN_FREQ_TRIM) driftIntegral = PLL_MIN_FREQ_TRIM;
    // limit correction range
    int32_t trim = offsetProportion + offsetIntegral;
    if(trim > PLL_MAX_FREQ_TRIM) trim = PLL_MAX_FREQ_TRIM;
    if(trim < PLL_MIN_FREQ_TRIM) trim = PLL_MIN_FREQ_TRIM;
    CLK_TAI_setTrim(trim);
}

void PLL_updateDrift(int interval, float drift) {
//    if(interval < 0) interval = 0;
//    float rate = 0x1p-16f * (float) (1 << (16 - interval));
//    driftIntegral += (int32_t) (0x1p32f * PLL_DRIFT_INT_RATE * rate * drift);
    driftIntegral += (int32_t) (0x1p32f * PLL_DRIFT_INT_RATE * drift);
    // limit integration range
    if(driftIntegral > PLL_MAX_FREQ_TRIM) driftIntegral = PLL_MAX_FREQ_TRIM;
    if(driftIntegral < PLL_MIN_FREQ_TRIM) driftIntegral = PLL_MIN_FREQ_TRIM;
    CLK_COMP_setComp(driftIntegral);
}
