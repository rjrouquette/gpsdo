//
// Created by robert on 4/27/23.
//

#include <memory.h>
#include "../chrony/candm.h"
#include "../clk/clk.h"
#include "../clk/mono.h"
#include "../clk/util.h"
#include "../gps.h"
#include "ref.h"

void NtpGPS_run(void *pObj) {
    struct NtpGPS *this = (struct NtpGPS *) pObj;

    uint64_t now = CLK_MONO();
    uint64_t pps = CLK_MONO_PPS();
    // is pps too old?
    if(now - pps > 0x140000000ull) {
        // advance reach indicator if the pulse has been missed
        if(((int64_t)(now - this->last_update)) > 0x100000000ull) {
            this->last_update = now;
            this->source.reach <<= 1;
        }
        return;
    }
    // wait for update
    if(pps == this->last_pps) return;
    this->last_update = now;
    this->source.freqDrift = 0x1p-32f * (float) (int32_t) (pps - this->last_pps);
    this->last_pps = pps;
    // update reach indicator
    this->source.reach = (this->source.reach << 1) | 1;
    // advance sample buffer
    NtpSource_incr(&this->source);
    // get current sample
    struct NtpPollSample *sample = this->source.pollSample + this->source.samplePtr;
    // retrieve timestamps
    CLK_PPS((uint64_t *) &sample->offset);
    // compute TAI offset
    union fixed_32_32 scratch;
    scratch.ipart = GPS_taiEpoch();
    scratch.fpart = 0;
    sample->offset.tai -= (int64_t) scratch.full;
    sample->delay = 0;
    sample->jitter = 4e-9f;
    // update filter
//    NtpSource_update(&this->source);
}

void NtpGPS_init(void *pObj) {
    // clear structure contents
    memset(pObj, 0, sizeof(struct NtpGPS));

    struct NtpGPS *this = (struct NtpGPS *) pObj;
    // set virtual functions
    this->source.init = NtpGPS_init;
    this->source.run = NtpGPS_run;
    // set mode
    this->source.mode = RPY_SD_MD_REF;
    // set id to "GPS"
    this->source.id = 0x00535047;
    // one second poll interval
    this->source.poll = 0;
}
