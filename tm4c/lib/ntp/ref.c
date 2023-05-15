//
// Created by robert on 4/27/23.
//

#include <memory.h>
#include "../chrony/candm.h"
#include "../clk/clk.h"
#include "../clk/mono.h"
#include "../clk/tai.h"
#include "../clk/util.h"
#include "../gps.h"
#include "common.h"
#include "ntp.h"
#include "ref.h"

static void NtpGPS_updateStatus(NtpGPS *this) {
    // update status
    if(this->source.reach == 0)
        this->source.lost = true;
    else if(this->source.reach == 0xFFFF)
        this->source.lost = false;
}

static void NtpGPS_run(volatile void *pObj) {
    NtpGPS *this = (struct NtpGPS *) pObj;

    // get pps timestamps
    uint64_t ppsTime[3];
    CLK_PPS(ppsTime);

    // is pps too old?
    const uint64_t now = CLK_MONO();
    if(now - ppsTime[0] > 0x140000000ull) {
        // advance reach indicator if the pulse has been missed
        if(((int64_t)(now - this->lastPoll)) > 0x100000000ull) {
            this->lastPoll = now;
            this->source.reach <<= 1;
            ++this->source.txCount;
            // update status
            NtpGPS_updateStatus(this);
        }
        return;
    }
    // GPS must have time set
    if(GPS_taiEpoch() == 0) return;
    // wait for update
    if(ppsTime[0] == this->lastPps) return;
    this->lastPoll = now;
    this->lastPps = ppsTime[0];

    // align GPS TAI epoch
    union fixed_32_32 scratch;
    scratch.full = ppsTime[0];
    scratch.full -= GPS_taiEpochUpdate();
    uint32_t taiEpoch = GPS_taiEpoch() + scratch.ipart + 1;

    // update reach indicator
    this->source.reach = (this->source.reach << 1) | 1;
    ++this->source.rxCount;
    ++this->source.rxValid;
    ++this->source.txCount;

    // update TAI/UTC offset
    scratch.ipart = GPS_taiOffset();
    scratch.fpart = 0;
    clkTaiUtcOffset = scratch.full;

    // advance sample buffer
    NtpSource_incr(&this->source);
    // get current sample
    NtpPollSample *sample = this->source.pollSample + this->source.samplePtr;
    // store PPS timestamps
    sample->comp = ppsTime[1];
    sample->tai = ppsTime[2];
    // compute TAI offset
    scratch.ipart = taiEpoch;
    scratch.fpart = 0;
    sample->offset = (int64_t) (scratch.full - ppsTime[2]);
    sample->delay = 0;

    // update filter
    NtpSource_update(&this->source);
    // update status
    NtpGPS_updateStatus(this);
}

void NtpGPS_init(volatile void *pObj) {
    // clear structure contents
    memset((void *) pObj, 0, sizeof(struct NtpGPS));

    NtpGPS *this = (NtpGPS *) pObj;
    // set virtual functions
    this->source.init = NtpGPS_init;
    this->source.run = NtpGPS_run;
    // set mode
    this->source.lost = true;
    this->source.mode = RPY_SD_MD_REF;
    this->source.state = RPY_SD_ST_UNSELECTED;
    this->source.version = 4;
    this->source.ntpMode = 4;
    // set id to "GPS"
    this->source.id = NTP_REF_GPS;
    this->source.precision = NTP_CLK_PREC;
    // one second poll interval
    this->source.poll = 0;
}
