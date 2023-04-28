//
// Created by robert on 4/27/23.
//

#ifndef GPSDO_SRC_H
#define GPSDO_SRC_H

#include <stdint.h>

#define NTP_MAX_HISTORY (16)

struct NtpPollSample {
    struct {
        int64_t mono;
        int64_t trim;
        int64_t tai;
    } offset;
    float delay;
    float jitter;
};

struct NtpSource {
    void (*init)(void *);
    void (*run)(void *);

    uint32_t id;
    uint32_t ref_id;
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
    // time span
    int span;
    int used;
    // frequency stats
    float freqDrift;
    float freqSkew;
};

void NtpSource_incr(struct NtpSource *this);

void NtpSource_update(struct NtpSource *this);

#endif //GPSDO_SRC_H
