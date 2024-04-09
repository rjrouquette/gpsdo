//
// Created by robert on 3/26/24.
//

#include "source.hpp"

#include "common.hpp"
#include "../run.hpp"
#include "../chrony/util.hpp"
#include "../clk/mono.h"
#include "../clk/util.hpp"
#include "../net/ip.hpp"
#include "../net/util.hpp"

#include <cmath>


static void getMeanVar(int cnt, const float *v, float *mean, float *var);

ntp::Source::Source(const uint32_t id_, const uint16_t mode_) :
    ringSamples{}, id(id_), mode(mode_) {
    // sample ring buffer
    ringPtr = 0;;
    sampleCount = 0;

    // samples used in calculations
    usedOffset = 0;
    usedDrift = 0;
    // time span
    span = 0;

    // polling state variable
    lastUpdate = 0;
    refTime = 0;
    refId = 0;
    rootDelay = 0;
    rootDispersion = 0;
    rxCount = 0;
    rxValid = 0;
    txCount = 0;
    responseTime = 0;
    state = 0;
    reach = 0;
    stratum = 0;
    poll = 0;
    pollCounter = 0;
    minPoll = 0;
    maxPoll = 0;
    precision = 0;
    leap = 0;
    version = 0;
    ntpMode = 0;

    // last sample offset
    lastOffset = 0;
    lastOffsetOrig = 0;
    lastDelay = 0;
    // offset stats
    offsetMean = 0;
    offsetStdDev = 0;
    // delay stats
    delayMean = 0;
    delayStdDev = 0;
    // frequency stats
    freqDrift = 0;
    freqSkew = 0;
    // overall score
    score = 0;

    // status flags
    xleave = false;
    prune = false;
    lost = false;
    unstable = false;

    // set initial state
    lost = true;
    state = RPY_SD_ST_UNSELECTED;
}

ntp::Source::~Source() {
    runCancel(nullptr, this);
    const_cast<uint32_t&>(id) = 0;
    const_cast<uint16_t&>(mode) = 0;
}

void ntp::Source::applyOffset(const int64_t offset) {
    for (auto &sample : ringSamples) {
        sample.taiSkew += offset;
        sample.offset -= offset;
    }
}

ntp::Source::Sample& ntp::Source::advanceFilter() {
    ringPtr = (ringPtr + 1) & (MAX_HISTORY - 1);
    if (sampleCount < MAX_HISTORY)
        ++sampleCount;
    return ringSamples[ringPtr];
}

void ntp::Source::updateFilter() {
    const auto &sample = ringSamples[ringPtr];
    lastOffsetOrig = toFloat(sample.offset);
    lastOffset = lastOffsetOrig;
    lastDelay = sample.delay;

    // compute time spanned by samples
    int j = (ringPtr - (sampleCount - 1)) & (MAX_HISTORY - 1);
    span = (int) ((sample.comp - ringSamples[j].comp) >> 32);

    // convert offsets to floats
    int index[MAX_HISTORY];
    float offset[MAX_HISTORY];
    float delay[MAX_HISTORY];
    int cnt = sampleCount;
    for (int i = 0; i < cnt; i++) {
        int k = (ringPtr - i) & (MAX_HISTORY - 1);
        index[i] = k;
        offset[i] = toFloat(ringSamples[k].offset);
        delay[i] = ringSamples[k].delay;
    }

    // compute mean and variance
    float mean, var;
    getMeanVar(sampleCount, delay, &mean, &var);
    delayMean = mean;
    delayStdDev = sqrtf(var);

    // compute mean and variance
    getMeanVar(sampleCount, offset, &mean, &var);
    // remove extrema
    float limit = var * 4;
    if (limit > 0) {
        j = 0;
        for (int i = 0; i < cnt; i++) {
            float diff = offset[i] - mean;
            if ((diff * diff) < limit) {
                index[j] = index[i];
                offset[j] = offset[i];
                ++j;
            }
        }
        cnt = j;
        // recompute mean and variance
        if (cnt > 0)
            getMeanVar(cnt, offset, &mean, &var);
    }
    // update offset stats
    usedOffset = cnt;
    offsetMean = mean;
    offsetStdDev = sqrtf(var);

    // analyse clock drift
    --cnt;
    float drift[cnt];
    for (int i = 0; i < cnt; i++) {
        const auto &current = ringSamples[index[i]];
        const auto &previous = ringSamples[index[i + 1]];

        // all three deltas are required to isolate drift from offset adjustments
        uint32_t a = current.taiSkew - previous.taiSkew;
        uint32_t b = static_cast<uint32_t>(current.offset) - static_cast<uint32_t>(previous.offset);
        drift[i] = 0x1p-32f * static_cast<float>(static_cast<int32_t>(a + b)) /
                   toFloatU(current.comp - previous.comp);
    }
    // compute mean and variance
    getMeanVar(cnt, drift, &mean, &var);
    // exclude outliers
    limit = var * 4;
    if (limit > 0) {
        j = 0;
        for (int i = 0; i < cnt; i++) {
            float diff = drift[i] - mean;
            if ((diff * diff) < limit) {
                drift[j] = drift[i];
                ++j;
            }
        }
        cnt = j;
        // recompute mean and variance
        if (cnt > 0)
            getMeanVar(cnt, drift, &mean, &var);
    }
    // set frequency status
    usedDrift = cnt;
    freqDrift = mean;
    freqSkew = sqrtf(var);
    // set overall score
    float score = std::abs(0x1p-16f * (float) rootDelay);
    score += 0x1p-16f * static_cast<float>(rootDispersion);
    score += delayMean;
    score += delayStdDev;
    score += offsetStdDev;
    this->score = score;

    // set update time
    lastUpdate = CLK_MONO();
}

bool ntp::Source::isSelectable() {
    // determine if the link has been lost
    if (lost) {
        state = RPY_SD_ST_UNSELECTED;
        return false;
    }

    // determine if the collected stats are erratic
    if (usedOffset < 8 || usedDrift < 4) {
        state = RPY_SD_ST_FALSETICKER;
        return false;
    }

    // determine if the connection is jittery
    if (freqSkew > MAX_FREQ_SKEW) {
        state = RPY_SD_ST_JITTERY;
        return false;
    }

    state = RPY_SD_ST_SELECTABLE;
    return true;
}

void ntp::Source::getNtpData(RPY_NTPData &rpyNtpData) const {
    rpyNtpData.local_addr.family = htons(IPADDR_INET4);
    rpyNtpData.local_addr.addr.in4 = ipAddress;
    rpyNtpData.remote_addr.family = htons(IPADDR_INET4);
    rpyNtpData.remote_addr.addr.in4 = id;
    rpyNtpData.remote_port = htons(NTP_PORT_SRV);
    rpyNtpData.mode = ntpMode;
    rpyNtpData.version = version;
    rpyNtpData.stratum = stratum;
    rpyNtpData.leap = leap;
    rpyNtpData.precision = precision;
    rpyNtpData.poll = static_cast<int8_t>(poll);

    rpyNtpData.ref_id = refId;
    chrony::toTimespec(htonll(refTime) - NTP_UTC_OFFSET, &(rpyNtpData.ref_time));

    rpyNtpData.offset.f = chrony::htonf(lastOffset);
    rpyNtpData.root_delay.f = chrony::htonf(0x1p-16f * static_cast<float>(rootDelay));
    rpyNtpData.root_dispersion.f = chrony::htonf(0x1p-16f * static_cast<float>(rootDispersion));
    rpyNtpData.peer_delay.f = chrony::htonf(delayMean);
    rpyNtpData.peer_dispersion.f = chrony::htonf(offsetStdDev);
    rpyNtpData.jitter_asymmetry.f = 0;
    rpyNtpData.response_time.f = chrony::htonf(responseTime);
    rpyNtpData.rx_tss_char = 'H';
    rpyNtpData.tx_tss_char = 'H';
    rpyNtpData.total_rx_count = htonl(rxCount);
    rpyNtpData.total_tx_count = htonl(txCount);
    rpyNtpData.total_valid_count = htonl(rxValid);
    if (xleave)
        rpyNtpData.flags |= htons(RPY_NTP_FLAG_INTERLEAVED);
}

void ntp::Source::getSourceStats(RPY_Sourcestats &rpySourceStats) const {
    // set source id
    if (isReference()) {
        rpySourceStats.ref_id = id;
        rpySourceStats.ip_addr.family = htons(IPADDR_UNSPEC);
        rpySourceStats.ip_addr.addr.in4 = id;
    }
    else {
        rpySourceStats.ref_id = refId;
        rpySourceStats.ip_addr.family = htons(IPADDR_INET4);
        rpySourceStats.ip_addr.addr.in4 = id;
    }

    rpySourceStats.n_samples = htonl(sampleCount);
    rpySourceStats.n_runs = htonl(usedOffset);
    rpySourceStats.span_seconds = htonl(span);
    rpySourceStats.est_offset.f = chrony::htonf(offsetMean);
    rpySourceStats.sd.f = chrony::htonf(offsetStdDev);
    rpySourceStats.resid_freq_ppm.f = chrony::htonf(freqDrift * 1e6f);
    rpySourceStats.skew_ppm.f = chrony::htonf(freqSkew * 1e6f);
}

void ntp::Source::getSourceData(RPY_Source_Data &rpySourceData) const {
    // set source id
    rpySourceData.mode = htons(mode);
    rpySourceData.ip_addr.family = htons(IPADDR_INET4);
    rpySourceData.ip_addr.addr.in4 = id;

    rpySourceData.stratum = htons(stratum);
    rpySourceData.reachability = htons(reach & 0xFF);
    rpySourceData.orig_latest_meas.f = chrony::htonf(lastOffsetOrig);
    rpySourceData.latest_meas.f = chrony::htonf(lastOffset);
    rpySourceData.latest_meas_err.f = chrony::htonf(lastDelay);
    rpySourceData.since_sample = htonl((CLK_MONO() - lastUpdate) >> 32);
    rpySourceData.poll = static_cast<int16_t>(htons(poll));
    rpySourceData.state = htons(state);
}

static void getMeanVar(const int cnt, const float *v, float *mean, float *var) {
    // return zeros if count is less than one
    if(cnt < 1) {
        *mean = 0;
        *var = 0;
        return;
    }

    // compute the mean
    float _mean = 0;
    for(int k = 0; k < cnt; k++)
        _mean += v[k];
    _mean /= static_cast<float>(cnt);

    // compute the variance
    float _var = 0;
    for(int k = 0; k < cnt; k++) {
        float diff = v[k] - _mean;
        _var += diff * diff;
    }
    _var /= static_cast<float>(cnt - 1);

    // return result
    *mean = _mean;
    *var = _var;
}
