//
// Created by robert on 4/29/23.
//

#include "../clk/comp.h"
#include "pll.h"
#include "../clk/tai.h"

// PLL integral terms
volatile float offsetIntegral;
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
    if(rmsOffset < PLL_OFFSET_CORR_BASIS)
        offsetIntegral += pllCorr + PLL_OFFSET_INT_RATE;
    CLK_TAI_setTrim((int32_t) (0x1p32f * (pllCorr + offsetIntegral)));
}

void PLL_updateDrift(int interval, float drift) {
//    if(interval < 0) interval = 0;
//    float rate = 0x1p-16f * (float) (1 << (16 - interval));
//    driftIntegral += (int32_t) (0x1p32f * PLL_DRIFT_INT_RATE * rate * drift);
    driftIntegral += (int32_t) (0x1p32f * PLL_DRIFT_INT_RATE * drift);
    CLK_COMP_setComp(driftIntegral);
}
