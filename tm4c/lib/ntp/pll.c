//
// Created by robert on 4/29/23.
//

#include <math.h>
#include "../clk/comp.h"
#include "../clk/tai.h"
#include "../format.h"
#include "pll.h"
#include "tcmp.h"


// offset statistics
static volatile float offsetLast;
static volatile float offsetMean;
static volatile float offsetVar;
static volatile float offsetMS;
static volatile float offsetStdDev;
static volatile float offsetRms;

// drift statistics
static volatile float driftLast;
static volatile float driftMean;
static volatile float driftVar;
static volatile float driftMS;
static volatile float driftStdDev;
static volatile float driftRms;

// PLL proportional terms
static volatile int32_t offsetProportion;

// PLL integral terms
static volatile int32_t offsetIntegral;
static volatile int32_t driftIntegral;

// Temperature Compensation State
static volatile float tcompCurrent;
static volatile float tcompPrior;
static volatile int32_t driftComp;

void PLL_init() {
    TCMP_init();

    tcompCurrent = TCMP_get();
    tcompPrior = tcompCurrent;
    driftComp = (int32_t) (0x1p32f * tcompCurrent);
    CLK_COMP_setComp(driftComp + driftIntegral);
}

void PLL_run() {
    TCMP_run();

    // get current tcomp value and compute delta
    tcompCurrent = TCMP_get();
    float tcompDelta = tcompCurrent - tcompPrior;
    tcompPrior = tcompCurrent;

    // apply tcomp update
    if (tcompDelta != 0) {
        if (fabsf(tcompDelta) > 100e-9f) {
            int32_t driftNew = (int32_t) (0x1p32f * tcompCurrent);
            driftIntegral -= driftNew - driftComp;
            driftComp = driftNew;
        } else {
            driftComp = (int32_t) (0x1p32f * tcompCurrent);
        }
        CLK_COMP_setComp(driftComp + driftIntegral);
    }
}


// from ntp.c
void ntpApplyOffset(int64_t offset);

static void ntpSetTaiClock(int64_t offset) {
    CLK_TAI_adjust(offset);
    ntpApplyOffset(offset);
}

void PLL_updateOffset(int interval, int64_t offset) {
    // apply hard correction to TAI clock for large offsets
    if((offset > PLL_OFFSET_HARD_ALIGN) || (offset < -PLL_OFFSET_HARD_ALIGN)) {
        ntpSetTaiClock(offset);
        offsetMS = 0;
        return;
    }

    float fltOffset = 0x1p-32f * (float) (int32_t) offset;
    offsetLast = fltOffset;
    if(offsetMS == 0) {
        // initialize stats
        offsetMean = fltOffset;
        offsetVar = fltOffset * fltOffset;
        offsetMS = offsetVar;
        offsetRms = fltOffset;
        offsetStdDev = fltOffset;
    } else {
        // update stats
        float diff = fltOffset - offsetMean;
        offsetVar += ((diff * diff) - offsetVar) * PLL_STATS_ALPHA;
        offsetMS += ((fltOffset * fltOffset) - offsetMS) * PLL_STATS_ALPHA;
        offsetMean += diff * PLL_STATS_ALPHA;
        offsetStdDev = sqrtf(offsetVar);
        offsetRms = sqrtf(offsetMS);
    }

    // compute proportional rate
    float rate = offsetRms / PLL_OFFSET_CORR_BASIS;
    // limit proportional rate
    if(rate > PLL_OFFSET_CORR_MAX) rate = PLL_OFFSET_CORR_MAX;
    // adjust rate to match polling interval
    rate *= 0x1p-16f * (float) (1u << (16 - interval));
    // update offset compensation
    float pllCorr = rate * fltOffset;
    // convert back to fixed-point correction factor
    offsetProportion = (int32_t) (0x1p32f * pllCorr);
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
    // update temperature compensation
    TCMP_update(drift + (0x1p-32f * (float) CLK_COMP_getComp()));

    // update stats
    driftLast = drift;
    float diff = drift - driftMean;
    driftVar += ((diff * diff) - driftVar) * PLL_STATS_ALPHA;
    driftMS += ((drift * drift) - driftMS) * PLL_STATS_ALPHA;
    driftMean += diff * PLL_STATS_ALPHA;
    driftStdDev = sqrtf(driftVar);
    driftRms = sqrtf(driftMS);

//    if(interval < 0) interval = 0;
//    float rate = 0x1p-16f * (float) (1 << (16 - interval));
//    driftIntegral += (int32_t) (0x1p32f * PLL_DRIFT_INT_RATE * rate * drift);
    driftIntegral += (int32_t) (0x1p32f * PLL_DRIFT_INT_RATE * drift);
    // limit integration range
    if(driftIntegral > PLL_MAX_FREQ_TRIM) driftIntegral = PLL_MAX_FREQ_TRIM;
    if(driftIntegral < PLL_MIN_FREQ_TRIM) driftIntegral = PLL_MIN_FREQ_TRIM;
    CLK_COMP_setComp(driftComp + driftIntegral);
}

unsigned PLL_status(char *buffer) {
    char tmp[32];
    char *end = buffer;

    end = append(end, "offset status:\n");

    tmp[fmtFloat(offsetMean * 1e6f, 12, 4, tmp)] = 0;
    end = append(end, "  - mean:  ");
    end = append(end, tmp);
    end = append(end, " us\n");

    tmp[fmtFloat(offsetStdDev * 1e6f, 12, 4, tmp)] = 0;
    end = append(end, "  - dev:   ");
    end = append(end, tmp);
    end = append(end, " us\n");

    tmp[fmtFloat(offsetRms * 1e6f, 12, 4, tmp)] = 0;
    end = append(end, "  - rms:   ");
    end = append(end, tmp);
    end = append(end, " us\n");

    tmp[fmtFloat(1e6f * 0x1p-32f * (float) offsetProportion, 12, 4, tmp)] = 0;
    end = append(end, "  - pll p: ");
    end = append(end, tmp);
    end = append(end, " ppm\n");

    tmp[fmtFloat(1e6f * 0x1p-32f * (float) offsetIntegral, 12, 4, tmp)] = 0;
    end = append(end, "  - pll i: ");
    end = append(end, tmp);
    end = append(end, " ppm\n\n");


    end = append(end, "drift status:\n");

    tmp[fmtFloat(driftMean * 1e6f, 12, 4, tmp)] = 0;
    end = append(end, "  - mean:  ");
    end = append(end, tmp);
    end = append(end, " ppm\n");

    tmp[fmtFloat(driftStdDev * 1e6f, 12, 4, tmp)] = 0;
    end = append(end, "  - dev:   ");
    end = append(end, tmp);
    end = append(end, " ppm\n");

    tmp[fmtFloat(driftRms * 1e6f, 12, 4, tmp)] = 0;
    end = append(end, "  - rms:   ");
    end = append(end, tmp);
    end = append(end, " ppm\n");

    tmp[fmtFloat(1e6f * 0x1p-32f * (float) driftIntegral, 12, 4, tmp)] = 0;
    end = append(end, "  - pll i: ");
    end = append(end, tmp);
    end = append(end, " ppm\n");

    tmp[fmtFloat(1e6f * 0x1p-32f * (float) driftComp, 12, 4, tmp)] = 0;
    end = append(end, "  - tcomp: ");
    end = append(end, tmp);
    end = append(end, " ppm\n\n");

    end += TCMP_status(end);

    return end - buffer;
}

// offset stats
float PLL_offsetLast() { return offsetLast; }
float PLL_offsetMean() { return offsetMean; }
float PLL_offsetRms() { return offsetRms; }
float PLL_offsetStdDev() { return offsetStdDev; }

// drift stats
float PLL_driftLast() { return driftLast; }
float PLL_driftMean() { return driftMean; }
float PLL_driftRms() { return driftRms; }
float PLL_driftStdDev() { return driftStdDev; }
