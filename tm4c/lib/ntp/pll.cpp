//
// Created by robert on 4/29/23.
//

#include "pll.hpp"

#include "tcmp.hpp"
#include "../format.hpp"
#include "../clock/comp.hpp"
#include "../clock/tai.hpp"

#include <cmath>


// PLL statistics slew rate
static constexpr float PLL_STATS_ALPHA = 0x1p-4f;
// maximum allowed frequecny triming (250 ppm)
static constexpr float PLL_MAX_FREQ_TRIM = 250e-6f;
// offset hard alignment threshold (39.06 ms)
static constexpr int64_t PLL_OFFSET_HARD_ALIGN = 0x01000000ll;
// unity rate threshold (1 ppm)
static constexpr float PLL_OFFSET_CORR_BASIS = 1e-6f;
// rate ceiling (dampens oscillation)
static constexpr float PLL_OFFSET_CORR_MAX = 0x1p-1f;
// rate floor (dampens oscillation)
static constexpr float PLL_OFFSET_CORR_MIN = 0x1p-3f;
// integration rate relative to proportional coefficient
static constexpr float PLL_OFFSET_INT_RATE = 0x1p-5f;

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
static volatile float driftFreq;

// PLL proportional terms
static volatile float offsetProportion;

// PLL integral terms
static volatile float offsetIntegral;

void PLL_init() {
    tcmp::init();
}


// from ntp.c
void ntpApplyOffset(int64_t offset);

static void ntpSetTaiClock(const int64_t offset) {
    clock::tai::adjust(offset);
    ntpApplyOffset(offset);
}

void PLL_updateOffset(const int interval, const int64_t offset) {
    // apply hard correction to TAI clock for large offsets
    if((offset > PLL_OFFSET_HARD_ALIGN) || (offset < -PLL_OFFSET_HARD_ALIGN)) {
        ntpSetTaiClock(offset);
        offsetMS = 0;
        return;
    }

    const float fltOffset = 0x1p-32f * static_cast<float>(static_cast<int32_t>(offset));
    offsetLast = fltOffset;
    if(offsetMS == 0) {
        // initialize stats
        offsetMean = fltOffset;
        offsetVar = fltOffset * fltOffset;
        offsetMS = offsetVar;
        offsetRms = fabsf(fltOffset);
        offsetStdDev = fabsf(fltOffset);
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
    if(rate < PLL_OFFSET_CORR_MIN) rate = PLL_OFFSET_CORR_MIN;
    // adjust rate to match polling interval
    rate *= 0x1p-16f * static_cast<float>(1u << (16 - interval));
    // update offset compensation
    offsetProportion = fltOffset * rate;
    offsetIntegral += offsetProportion * PLL_OFFSET_INT_RATE;
    // limit integration range
    if(offsetIntegral >  PLL_MAX_FREQ_TRIM) offsetIntegral =  PLL_MAX_FREQ_TRIM;
    if(offsetIntegral < -PLL_MAX_FREQ_TRIM) offsetIntegral = -PLL_MAX_FREQ_TRIM;
    // limit correction range
    float trim = offsetProportion + offsetIntegral;
    if(trim >  PLL_MAX_FREQ_TRIM) trim =  PLL_MAX_FREQ_TRIM;
    if(trim < -PLL_MAX_FREQ_TRIM) trim = -PLL_MAX_FREQ_TRIM;
    clock::tai::setTrim(static_cast<int32_t>(0x1p32f * trim));
}

void PLL_updateDrift(int interval, const float drift) {
    // update stats
    driftLast = drift;
    const float diff = drift - driftMean;
    driftVar += ((diff * diff) - driftVar) * PLL_STATS_ALPHA;
    driftMS += ((drift * drift) - driftMS) * PLL_STATS_ALPHA;
    driftMean += diff * PLL_STATS_ALPHA;
    driftStdDev = sqrtf(driftVar);
    driftRms = sqrtf(driftMS);

    // update temperature compensation
    driftFreq = drift + (0x1p-32f * static_cast<float>(clock::compensated::getTrim()));
    tcmp::update(driftFreq, 100e-9f / (100e-9f + driftStdDev + fabsf(diff)));
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

    tmp[fmtFloat(offsetProportion * 1e6f, 12, 4, tmp)] = 0;
    end = append(end, "  - pll p: ");
    end = append(end, tmp);
    end = append(end, " ppm\n");

    tmp[fmtFloat(offsetIntegral * 1e6f, 12, 4, tmp)] = 0;
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
    end = append(end, " ppm\n\n");

    end += tcmp::status(end);

    return end - buffer;
}

// offset stats
float PLL_offsetLast() { return offsetLast; }
float PLL_offsetMean() { return offsetMean; }
float PLL_offsetRms() { return offsetRms; }
float PLL_offsetStdDev() { return offsetStdDev; }
float PLL_offsetProp() { return offsetProportion; }
float PLL_offsetInt() { return offsetIntegral; }
float PLL_offsetCorr() { return 0x1p-32f * static_cast<float>(clock::tai::getTrim()); }

// drift stats
float PLL_driftLast() { return driftLast; }
float PLL_driftMean() { return driftMean; }
float PLL_driftRms() { return driftRms; }
float PLL_driftStdDev() { return driftStdDev; }
float PLL_driftCorr() { return 0x1p-32f * static_cast<float>(clock::compensated::getTrim()); }
float PLL_driftFreq() { return driftFreq; }
