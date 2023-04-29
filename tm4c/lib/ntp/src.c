//
// Created by robert on 4/27/23.
//

#include <math.h>
#include "../clk/util.h"
#include "src.h"

static void getMeanVar(int cnt, const float *v, float *mean, float *var);

void NtpSource_incr(NtpSource *this) {
    this->samplePtr = (this->samplePtr + 1) & (NTP_MAX_HISTORY - 1);
    if(++this->sampleCount > NTP_MAX_HISTORY)
        this->sampleCount = NTP_MAX_HISTORY;
}

void NtpSource_update(NtpSource *this) {
    const int head = this->samplePtr;
    NtpPollSample *sample = this->pollSample + head;
    this->lastOffsetOrig = toFloat(sample->offset);
    this->lastOffset = this->lastOffsetOrig;
    this->lastDelay = sample->delay;

    int j = head - this->sampleCount + 1;
    if (j < 0) j += NTP_MAX_HISTORY;
    uint32_t span = sample->comp >> 32;
    span -= this->pollSample[j].comp >> 32;
    this->span = (int) span;

    // convert offsets to floats
    int index[NTP_MAX_HISTORY];
    float offset[NTP_MAX_HISTORY];
    float delay[NTP_MAX_HISTORY];
    int cnt = this->sampleCount;
    for (int i = 0; i < cnt; i++) {
        int k = (head - i) & (NTP_MAX_HISTORY - 1);
        index[i] = k;
        offset[i] = toFloat(this->pollSample[k].offset);
        delay[i] = this->pollSample[k].delay;
    }

    // compute mean and variance
    float mean, var;
    getMeanVar(this->sampleCount, delay, &mean, &var);
    this->delayMean = mean;
    this->delayStdDev = sqrtf(var);

    // compute mean and variance
    getMeanVar(this->sampleCount, offset, &mean, &var);
    // remove extrema
    float limit = var * 4;
    j = 0;
    for (int i = 0; i < cnt; i++) {
        float diff = offset[i] - mean;
        if((diff * diff) < limit) {
            index[j] = index[i];
            offset[j] = offset[i];
            ++j;
        }
    }
    cnt = j;
    // recompute mean and variance
    if(cnt > 0)
        getMeanVar(cnt, offset, &mean, &var);
    // update offset stats
    this->used = cnt;
    this->offsetMean = mean;
    this->offsetStdDev = sqrtf(var);

    // skip drift calculation if only one sample
    if(cnt < 2) {
        this->freqDrift = 0;
        this->freqSkew = 0;
        return;
    }

    // analyse clock drift
    float drift[cnt - 1];
    for (int i = 0; i < cnt - 1; i++) {
        NtpPollSample *current = this->pollSample + index[i];
        NtpPollSample *previous = this->pollSample + index[i+1];

        int64_t a = (int64_t) (current->tai - previous->tai);
        int64_t b = (int64_t) (current->comp - previous->comp);
        int64_t c = (int64_t) (current->offset - previous->offset);
        drift[i] = toFloat(c + a - b) / toFloat(b);
    }
    // compute mean and variance
    getMeanVar(cnt - 1, drift, &mean, &var);
    // set frequency status
    this->freqDrift = mean;
    this->freqSkew = sqrtf(var);
    // set overall score
    float score = fabsf(0x1p-16f * (float) this->rootDelay);
    score += 0x1p-16f * (float) this->rootDispersion;
    score += this->delayMean;
    score += this->delayStdDev;
    this->score = score;
}

static void getMeanVar(int cnt, const float *v, float *mean, float *var) {
    float _mean = 0, _var = 0;
    for(int k = 0; k < cnt; k++)
        _mean += v[k];
    _mean /= (float) cnt;
    for(int k = 0; k < cnt; k++) {
        float diff = v[k] - _mean;
        _var += diff * diff;
    }
    _var /= (float) cnt;
    // return result
    *mean = _mean;
    *var = _var;
}
