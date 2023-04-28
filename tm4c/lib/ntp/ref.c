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

    // get current timestamp
    uint64_t now = CLK_MONO();
    // get pps timestamps
    uint64_t ppsTime[3];
    CLK_PPS(ppsTime);

    // is pps too old?
    if(now - ppsTime[0] > 0x140000000ull) {
        // advance reach indicator if the pulse has been missed
        if(((int64_t)(now - this->last_update)) > 0x100000000ull) {
            this->last_update = now;
            this->source.reach <<= 1;
        }
        return;
    }
    // wait for update
    if(ppsTime[0] == this->last_pps) return;
    this->last_update = now;
    this->source.freqDrift = 0x1p-32f * (float) (int32_t) (ppsTime[0] - this->last_pps);
    this->last_pps = ppsTime[0];
    // update reach indicator
    this->source.reach = (this->source.reach << 1) | 1;
    // advance sample buffer
    NtpSource_incr(&this->source);
    // get current sample
    struct NtpPollSample *sample = this->source.pollSample + this->source.samplePtr;
    // store PPS timestamps
    sample->offset.mono = (int64_t) ppsTime[0];
    sample->offset.trim = (int64_t) ppsTime[1];
    sample->offset.tai = (int64_t) ppsTime[2];
    // compute TAI offset
    union fixed_32_32 scratch;
    scratch.ipart = GPS_taiEpoch();
    scratch.fpart = 0;
    sample->offset.tai -= (int64_t) scratch.full;
    sample->delay = 0;
    sample->jitter = 4e-9f;
    // update filter
    NtpSource_update(&this->source);
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
