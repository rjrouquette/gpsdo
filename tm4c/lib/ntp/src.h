//
// Created by robert on 4/27/23.
//

#ifndef GPSDO_SRC_H
#define GPSDO_SRC_H

#include <stdint.h>
#include <stdbool.h>

#define NTP_MAX_HISTORY (16)
#define NTP_MAX_STRAT (3)
#define NTP_MAX_DELAY (50e-3f)

struct NtpPollSample {
    int64_t offset;
    float delay;
    uint32_t taiSkew;
    uint64_t comp;
};
typedef struct NtpPollSample NtpPollSample;

struct NtpSource {
    // filter samples
    struct NtpPollSample pollSample[NTP_MAX_HISTORY];
    uint8_t samplePtr;
    uint8_t sampleCount;
    uint8_t usedOffset;
    uint8_t usedDrift;
    // time span
    int span;

    uint64_t lastUpdate;
    uint64_t refTime;
    uint32_t id;
    uint32_t refID;
    uint32_t rootDelay;
    uint32_t rootDispersion;
    uint32_t rxCount;
    uint32_t rxValid;
    uint32_t txCount;
    float responseTime;
    uint16_t mode;
    uint16_t state;
    uint16_t reach;
    uint16_t stratum;
    int16_t poll;
    uint16_t pollCounter;
    int16_t minPoll;
    int16_t maxPoll;
    int8_t precision;
    uint8_t leap;
    uint8_t version;
    uint8_t ntpMode;

    // last sample offset
    float lastOffset;
    float lastOffsetOrig;
    float lastDelay;
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

    // status flags
    bool xleave;
    bool prune;
    bool lost;
    bool unstable;
};
typedef struct NtpSource NtpSource;

/**
 * Advance source sample pointer
 * @param this pointer to source structure
 */
void NtpSource_incr(NtpSource *this);

/**
 * Update source statistics
 * @param this pointer to source structure
 */
void NtpSource_update(NtpSource *this);

/**
 * Update source connectivity status
 * @param this pointer to source structure
 */
void NtpSource_updateStatus(NtpSource *this);

/**
 * Apply offset correction to samples
 * @param this pointer to source structure
 * @param offset correction to apply
 */
void NtpSource_applyOffset(NtpSource *this, int64_t offset);

#endif //GPSDO_SRC_H
