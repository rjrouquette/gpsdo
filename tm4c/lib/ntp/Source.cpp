//
// Created by robert on 3/26/24.
//

#include "Source.hpp"

#include "common.hpp"
#include "../run.hpp"
#include "../chrony/util.hpp"
#include "../clock/mono.hpp"
#include "../clock/util.hpp"
#include "../net/ip.hpp"
#include "../net/util.hpp"

#include <cmath>


static void fitLinear(
    int cnt,
    const float *x,
    const float *y,
    const float *w,
    float &offset,
    float &slope,
    float &offsetVariance,
    float &slopeVariance
);

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
    filteredOffset = 0;
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
    const_cast<volatile uint16_t&>(mode) = 0;
    const_cast<volatile uint32_t&>(id) = 0;
}

void ntp::Source::applyOffset(const int64_t offset) {
    for (auto &sample : ringSamples)
        sample.taiLocal += offset;
}

ntp::Source::Sample& ntp::Source::advanceFilter() {
    ringPtr = (ringPtr + 1) & RING_MASK;
    if (sampleCount < MAX_HISTORY)
        ++sampleCount;
    return ringSamples[ringPtr];
}

void ntp::Source::updateFilter() {
    updateDelay();

    const auto &sample = ringSamples[ringPtr];
    lastOffsetOrig = toFloat(sample.getOffset());
    lastOffset = lastOffsetOrig;
    lastDelay = sample.delay;

    // compute time spanned by samples
    span = static_cast<int>(
        (sample.taiLocal - ringSamples[(ringPtr - (sampleCount - 1)) & RING_MASK].taiLocal) >> 32
    );

    // convert offsets to floats
    float x[sampleCount];
    float y[sampleCount];
    float w[sampleCount];
    const auto lastOffset = ringSamples[ringPtr].getOffset();
    const auto lastLocal = ringSamples[ringPtr].taiLocal;
    auto delayVariance = delayStdDev + CLK_NANOS * 1e-9f;
    delayVariance *= delayVariance;
    for (int i = 0; i < sampleCount; i++) {
        const int k = (ringPtr - i) & RING_MASK;
        x[i] = toFloatU(lastLocal - ringSamples[k].taiLocal);
        y[i] = toFloat(ringSamples[k].getOffset() - lastOffset);
        const auto jitter = ringSamples[k].delay - delayMean;
        w[i] = delayVariance / (delayVariance + jitter * jitter);
    }

    float offsetVar, freqVar;
    fitLinear(sampleCount, x, y, w, offsetMean, freqDrift, offsetVar, freqVar);
    if (sampleCount > 4) {
        // reweight by offset error if there are sufficient samples
        offsetVar += CLK_NANOS * 1e-9f * CLK_NANOS * 1e-9f;
        for (int i = 0; i < sampleCount; i++) {
            const float error = y[i] - offsetMean - x[i] * freqDrift;
            w[i] = offsetVar / (offsetVar + error * error);
            fitLinear(sampleCount, x, y, w, offsetMean, freqDrift, offsetVar, freqVar);
        }
    }
    filteredOffset = lastOffset;// + toFixedPoint(offsetMean);
    offsetMean += toFloat(lastOffset);
    offsetStdDev = std::sqrt(offsetVar);
    freqSkew = std::sqrt(freqVar);

    float weight = 0;
    for (int i = 0; i < sampleCount; i++)
        weight += w[i];
    int count = static_cast<int>(std::ceil(weight));
    usedOffset = count;
    usedDrift = count;

    // set overall score
    float score = std::abs(0x1p-16f * static_cast<float>(rootDelay));
    score += 0x1p-16f * static_cast<float>(rootDispersion);
    score += delayMean;
    score += delayStdDev;
    score += offsetStdDev;
    this->score = score;

    // set update time
    lastUpdate = clock::monotonic::now();
}

void ntp::Source::updateDelay() {
    float delay[sampleCount];
    for (int i = 0; i < sampleCount; i++) {
        const int k = (ringPtr - i) & RING_MASK;
        delay[i] = ringSamples[k].delay;
    }

    // compute mean
    float mean = 0;
    for (const auto value : delay)
        mean += value;
    mean /= static_cast<float>(sampleCount);

    // compute variance
    float var = 0;
    for (const auto value : delay) {
        const auto diff = value - mean;
        var += diff * diff;
    }
    var /= static_cast<float>(sampleCount - 1);

    // update fields
    delayMean = mean;
    delayStdDev = sqrtf(var);
}

bool ntp::Source::isSelectable() {
    // determine if the link has been lost
    if (reach == 0 || lost) {
        state = RPY_SD_ST_NONSELECTABLE;
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
    rpySourceData.latest_meas_err.f = chrony::htonf(delayMean + delayStdDev);
    rpySourceData.since_sample = htonl((clock::monotonic::now() - lastUpdate) >> 32);
    rpySourceData.poll = static_cast<int16_t>(htons(poll));
    rpySourceData.state = htons(state);
}

static void fitLinear(
    int cnt,
    const float *x,
    const float *y,
    const float *w,
    float &offset,
    float &slope,
    float &offsetVariance,
    float &slopeVariance
) {
    float scratch[5];
    for (auto &v : scratch)
        v = 0;

    for (int i = 0; i < cnt; ++i) {
        scratch[0] += w[i];
        scratch[1] += w[i] * x[i];
        scratch[2] += w[i] * x[i] * x[i];

        scratch[3] += w[i] * y[i];
        scratch[4] += w[i] * y[i] * x[i];
    }

    for (int i = 1; i < 5; ++i)
        scratch[i] /= scratch[0];

    slope = (scratch[4] - scratch[1] * scratch[3]) / (scratch[2] - scratch[1] * scratch[1]);
    offset = scratch[3] - scratch[1] * slope;

    float mse = 0;
    for (int i = 0; i < cnt; ++i) {
        const float error = y[i] - offset - x[i] * slope;
        mse += w[i] * error * error;
    }
    mse /= scratch[0];

    slopeVariance = mse / (scratch[2] - scratch[1] * scratch[1]);
    offsetVariance = slopeVariance * scratch[2];
}
