//
// Created by robert on 4/27/23.
//

#include <math.h>
#include "../clk/mono.h"
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

    // compute time spanned by samples
    int j = (head - (this->sampleCount - 1)) & (NTP_MAX_HISTORY - 1);
    this->span = (int) ((sample->comp - this->pollSample[j].comp) >> 32);

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
    if(limit > 0) {
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
    this->used = cnt;
    this->offsetMean = mean;
    this->offsetStdDev = sqrtf(var);

    // analyse clock drift
    --cnt;
    float drift[cnt];
    for (int i = 0; i < cnt; i++) {
        NtpPollSample *current = this->pollSample + index[i];
        NtpPollSample *previous = this->pollSample + index[i+1];

        // all three deltas are required to isolate drift from offset adjustments
        int64_t a = (int64_t) (current->tai - previous->tai);
        int64_t b = (int64_t) (current->comp - previous->comp);
        int64_t c = (int64_t) (current->offset - previous->offset);
        drift[i] = toFloat(c + a - b) / toFloat(b);
    }
    // compute mean and variance
    getMeanVar(cnt, drift, &mean, &var);
    // exclude outliers
    limit = var * 4;
    if(limit > 0) {
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
    this->freqUsed = cnt;
    this->freqDrift = mean;
    this->freqSkew = sqrtf(var);
    // set overall score
    float score = fabsf(0x1p-16f * (float) this->rootDelay);
    score += 0x1p-16f * (float) this->rootDispersion;
    score += this->delayMean;
    score += this->delayStdDev;
    score += this->offsetStdDev;
    this->score = score;

    // set update time
    this->lastUpdate = CLK_MONO();
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
    _mean /= (float) cnt;

    // compute the variance
    float _var = 0;
    for(int k = 0; k < cnt; k++) {
        float diff = v[k] - _mean;
        _var += diff * diff;
    }
    _var /= (float) (cnt - 1);

    // return result
    *mean = _mean;
    *var = _var;
}


void NtpSource_updateStatus(NtpSource *this) {
    // clear lost flag if peer was reached
    if(this->reach & 0xF)
        this->lost = false;

    // increment poll counter
    if(++this->pollCounter == 0)
        this->pollCounter = -1;

    // adjust poll interval
    if(
            this->sampleCount == NTP_MAX_HISTORY &&
            (this->reach & 0xFF) == 0xFF &&
            this->pollCounter >= 8 &&
            this->poll < this->maxPoll
    ) {
        // reset the counter
        this->pollCounter = 0;
        // increase poll interval (increase update rate)
        ++this->poll;
        // sanity check
        if(this->poll < this->minPoll)
            this->poll = this->minPoll;
    }
    else if(
            (this->reach & 0xF) == 0 &&
            this->pollCounter >= 4
    ) {
        // mark association as lost
        this->lost = true;
        // reset the counter
        this->pollCounter = 0;
        // decrease poll interval (increase update rate)
        if(this->poll > this->minPoll)
            --this->poll;
    }

    // mark source for pruning if it is unreachable
    this->prune = (this->reach == 0) && (this->pollCounter >= 16);
    // mark source for pruning if its stratum is too high
    this->prune |= this->stratum > NTP_MAX_STRAT;
    // mark source for pruning if its delay is too high
    this->prune |= (this->used > 7) && (this->delayMean > NTP_MAX_DELAY);
}

void NtpSource_applyOffset(NtpSource *this, int64_t offset) {
    for (int i = 0; i < NTP_MAX_HISTORY; i++) {
        this->pollSample[i].tai += offset;
        this->pollSample[i].offset -= offset;
    }
}
