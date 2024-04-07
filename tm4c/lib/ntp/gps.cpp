//
// Created by robert on 3/26/24.
//

#include "gps.hpp"
#include "../gps.h"

#include "common.hpp"
#include "ntp.hpp"
#include "../run.h"
#include "../clk/clk.h"
#include "../clk/mono.h"
#include "../clk/tai.h"
#include "../clk/util.h"


ntp::GPS::GPS() :
    Source(NTP_REF_GPS, RPY_SD_MD_REF) {
    // clear PPS status
    lastPps = 0;
    lastPoll = 0;
    // set metadata
    version = 4;
    ntpMode = 4;
    precision = NTP_CLK_PREC;
    // one second poll interval
    maxPoll = 0;
    minPoll = 0;
    poll = 0;

    // run updates at 16Hz
    runSleep(RUN_SEC >> 4, &run, this);
}

ntp::GPS::~GPS() {
    Source::~Source();
}

void ntp::GPS::run(void *ref) {
    static_cast<GPS*>(ref)->run();
}

void ntp::GPS::run() {
    // get pps timestamps
    uint64_t ppsTime[3];
    CLK_PPS(ppsTime);

    // is pps too old?
    const uint64_t now = CLK_MONO();
    if (now - ppsTime[0] > 0x140000000ull) {
        // advance reach indicator if the pulse has been missed
        if (static_cast<int64_t>(now - lastPoll) > 0x100000000ll) {
            lastPoll = now;
            reach <<= 1;
            ++txCount;
            // update status
            updateStatus();
        }
        return;
    }
    // GPS must have time set
    if (GPS_taiEpoch() == 0)
        return;
    // wait for update
    if (ppsTime[0] == lastPps)
        return;
    // record update time
    lastPoll = now;
    lastPps = ppsTime[0];

    // update reach indicator
    reach = (reach << 1) | 1;
    ++rxCount;
    ++rxValid;
    ++txCount;

    // update TAI/UTC offset
    fixed_32_32 scratch = {};
    scratch.fpart = 0;
    scratch.ipart = static_cast<uint32_t>(GPS_taiOffset());
    clkTaiUtcOffset = scratch.full;

    // advance sample buffer
    auto &sample = advanceFilter();
    // store PPS timestamps
    sample.comp = ppsTime[1];
    sample.taiSkew = ppsTime[2] - ppsTime[1];
    // compute TAI offset
    scratch.full = ppsTime[0];
    scratch.full -= GPS_taiEpochUpdate();
    scratch.ipart += GPS_taiEpoch() + 1;
    scratch.fpart = 0;
    sample.offset = static_cast<int64_t>(scratch.full - ppsTime[2]);
    sample.delay = 0;

    // update filter
    updateFilter();
    // update status
    updateStatus();
}

void ntp::GPS::updateStatus() {
    // update status
    if (reach == 0)
        lost = true;
    else if (reach == 0xFFFF)
        lost = false;
}
