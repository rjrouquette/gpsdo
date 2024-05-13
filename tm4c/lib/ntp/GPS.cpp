//
// Created by robert on 3/26/24.
//

#include "GPS.hpp"
#include "../gps.hpp"

#include "common.hpp"
#include "../run.hpp"
#include "../clock/capture.hpp"
#include "../clock/mono.hpp"
#include "../clock/tai.hpp"
#include "../clock/util.hpp"


ntp::GPS::GPS() :
    Source(REF_ID, RPY_SD_MD_REF) {
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
    runSleep(RUN_SEC / 16, &run, this);
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
    clock::capture::ppsGps(ppsTime);

    // is pps too old?
    const uint64_t now = clock::monotonic::now();
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
    if (gps::taiEpoch() == 0)
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
    scratch.ipart = static_cast<uint32_t>(gps::taiOffset());
    clkTaiUtcOffset = scratch.full;

    // advance sample buffer
    auto &sample = advanceFilter();
    // store PPS timestamps
    sample.taiLocal = ppsTime[2];
    // compute TAI offset
    scratch.full = ppsTime[0];
    scratch.full -= gps::taiEpochUpdate();
    scratch.ipart += gps::taiEpoch() + 1;
    scratch.fpart = 0;
    sample.taiRemote = scratch.full;
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
