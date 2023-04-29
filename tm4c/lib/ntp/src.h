//
// Created by robert on 4/27/23.
//

#ifndef GPSDO_SRC_H
#define GPSDO_SRC_H

#include <stdint.h>

#define NTP_MAX_HISTORY (16)

struct NtpPollSample {
    uint64_t comp;
    uint64_t tai;
    int64_t offset;
    float delay;
};
typedef volatile struct NtpPollSample NtpPollSample;

struct NtpSource {
    void (*init)(volatile void *);
    void (*run)(volatile void *);

    uint64_t lastUpdate;
    uint32_t id;
    uint32_t ref_id;
    uint32_t rootDelay;
    uint32_t rootDispersion;
    uint16_t mode;
    uint16_t state;
    uint16_t reach;
    uint16_t stratum;
    int16_t poll;

    // filter state
    struct NtpPollSample pollSample[NTP_MAX_HISTORY];
    int samplePtr;
    int sampleCount;
    // last sample offset
    float lastOffset;
    float lastOffsetOrig;
    float lastDelay;
    // time span
    int span;
    int used;
    // offset stats
    float offsetMean;
    float offsetStdDev;
    // delay stats
    float delayMean;
    float delayStdDev;
    // frequency stats
    float freqDrift;
    float freqSkew;
    // overall score
    float score;
};
typedef volatile struct NtpSource NtpSource;

void NtpSource_incr(NtpSource *this);

void NtpSource_update(NtpSource *this);

#endif //GPSDO_SRC_H
