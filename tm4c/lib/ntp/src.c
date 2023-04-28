//
// Created by robert on 4/27/23.
//

#include <math.h>
#include "../clk/util.h"
#include "src.h"

void NtpSource_incr(struct NtpSource *this) {
    this->samplePtr = (this->samplePtr + 1) & (NTP_MAX_HISTORY - 1);
    if(++this->sampleCount > NTP_MAX_HISTORY)
        this->sampleCount = NTP_MAX_HISTORY;
}

void NtpSource_update(struct NtpSource *this) {
    int i = this->samplePtr;
    struct NtpPollSample *sample = this->pollSample + i;
    this->lastOffsetOrig = toFloat(sample->offset.tai);
    this->lastOffset = this->lastOffsetOrig;

    int j = i - this->sampleCount + 1;
    if (j < 0) j += NTP_MAX_HISTORY;
    uint32_t span = sample->offset.mono >> 32;
    span -= this->pollSample[j].offset.mono >> 32;
    this->span = (int) span;

    // analyse clock drift
    float drift[NTP_MAX_HISTORY - 1];
    float mean = 0, var = 0;
    struct NtpPollSample *future = this->pollSample + i;
    const int len = this->sampleCount + 1;
    for (int k = 0; k < len; k++) {
        i = (i - 1) & (NTP_MAX_HISTORY - 1);
        struct NtpPollSample *present = this->pollSample + i;

        int64_t a = (int64_t) (future->offset.tai - present->offset.tai);
        int64_t b = (int64_t) (future->offset.mono - present->offset.mono);
        drift[k] = toFloat(a) / toFloat(b);
        mean += drift[k];
        var += drift[k] * drift[k];
    }
    mean /= (float) len;
    var /= (float) len;
    var -= mean * mean;
    float limit = var * 4;
    float mu = mean;
    mean = 0;
    var = 0;
    int cnt = 0;
    for (int k = 0; k < len; k++) {
        float diff = drift[k] - mu;
        if((diff * diff) < limit) {
            mean += drift[k];
            var += drift[k] * drift[k];
            ++cnt;
        }
    }
    if(cnt > 0) {
        mean /= (float) cnt;
        var /= (float) cnt;
        var -= mean * mean;
    }
    this->freqDrift = mean;
    this->freqSkew = sqrtf(var);
    this->used = cnt;
}
