//
// Created by robert on 9/13/22.
//

#include <math.h>
#include <strings.h>

#include "qnorm.h"


static inline void updateBin(struct QNormBin *bin, float mu, float x, float w) {
    bin->tau += w;
    bin->mean += (x - mu) * w;
}

static void compUpdate(struct QNorm *qnorm, float x, struct QNormBin *acc) {
    struct QNormBin *bins = qnorm->bins;
    // locate pivot bin
    int i = QNORM_SIZE / 2;
    if(x > bins[i].mean) {
        // ascending search
        while (++i < QNORM_SIZE-1 && x > bins[i].mean);
    } else {
        // descending search
        while (--i > 0 && x <= bins[i].mean);
        ++i;
    }
    // locate nearest bin
    if((bins[i].mean - x) > (x - bins[i-1].mean)) --i;

    // update nearest bin
    updateBin(acc + i, bins[i].mean, x, 1.0f);
    // update neighboring bins
    if(i > 0           ) updateBin(acc + i-1, bins[i-1].mean, x, 0x1p-3f);
    if(i < QNORM_SIZE-1) updateBin(acc + i+1, bins[i+1].mean, x, 0x1p-3f);
    // update neighboring bins
    if(i > 1           ) updateBin(acc + i-2, bins[i-2].mean, x, 0x1p-10f);
    if(i < QNORM_SIZE-2) updateBin(acc + i+2, bins[i+2].mean, x, 0x1p-10f);
}

static void sort(float *begin, float *end);


void qnorm_clear(struct QNorm *qnorm) {
    bzero(qnorm, sizeof(struct QNorm));
}

void qnorm_init(struct QNorm *qnorm, const float * const data, const int count, const int stride) {
    qnorm_clear(qnorm);

    float sorted[count];
    const float *_data = data;
    for(int i = 0; i < count; i++) {
        sorted[i] = *_data;
        _data += stride;
    }

    // sort data points
    sort(sorted, sorted + count);

    // remove duplicate data points
    int i = 0;
    int j = 0;
    while(i < count) {
        const float v = sorted[i++];
        while(i < count) {
            if(sorted[i] == v) {
                i++;
            }
            else {
                break;
            }
        }
        sorted[j++] = v;
    }

    // select initial partition points
    for(i = 0; i < QNORM_SIZE; i++) {
        // clear bin tau
        qnorm->bins[i].tau = 0;
        // compute partition bounds
        float f = (float)i * ((float)j - 1.0f) / (QNORM_SIZE - 1.0f);
        float a = floorf(f), b = ceilf(f);
        // set initial bin mean
        if(a == b) qnorm->bins[i].mean = sorted[(int)a];
        else       qnorm->bins[i].mean = ((b - f) * sorted[(int)a]) + ((f - a) * sorted[(int)b]);
    }

    // update bins with original data samples
    float scratch[2];
    qnorm_update(qnorm, data, count, stride, scratch + 0, scratch + 1);
}

int qnorm_update(struct QNorm *qnorm, const float *data, int count, int stride, float *dOffset, float *dScale) {
    struct QNormBin acc[QNORM_SIZE];
    bzero(acc, sizeof(acc));

    // update bins with data samples
    for(int i = 0; i < count; i++) {
        compUpdate(qnorm, *data, acc);
        data += stride;
    }

    // update global tau
    if(qnorm->tau < QNORM_LIM) {
        qnorm->tau += (float) count;
        if (qnorm->tau > QNORM_LIM)
            qnorm->tau = QNORM_LIM;
    }

    // compute eta
    const float eta = 1.0f / qnorm->tau;
    float nrm = 0;
    for(int i = 0; i < QNORM_SIZE; i++) {
        struct QNormBin *bin = qnorm->bins + i;
        if(acc[i].tau > 0) {
            bin->tau += eta * acc[i].tau;
            bin->mean += eta * acc[i].mean / bin->tau;
        }
        nrm += bin->tau;
    }

    // only update scaling if sufficient entropy has accumulated
    if(nrm < 1.05f) {
        // no change to scaling transform
        *dScale = 1;
        *dOffset = 0;
        return 0;
    }

    // normalize bin taus and compute new offset
    nrm = 1.0f / nrm;
    float offset = 0;
    for(int i = 0; i < QNORM_SIZE; i++) {
        struct QNormBin *bin = qnorm->bins + i;
        bin->tau *= nrm;
        offset += bin->mean * bin->tau;
    }

    // compute correlation
    float z = 0, m = 0, n = 0;
    for(int i = 0; i < QNORM_SIZE; i++) {
        struct QNormBin *bin = qnorm->bins + i;

        float dx = z + (0.5f * bin->tau) - 0.5f;
        float dy = bin->mean - offset;

        m += bin->tau * dx * dy;
        n += bin->tau * dx * dx;
        z += bin->tau;
    }

    // compute scale and offset translation from old bounds to new bounds
    float rscale = 0.3f * m / n;
    *dScale  =  qnorm->rscale / rscale;
    *dOffset = (qnorm->offset - offset) / rscale;
    // set new scale
    qnorm->offset = offset;
    qnorm->tscale = 1.0f / rscale;
    qnorm->rscale = rscale;
    return 1;
}

float qnorm_transform(const struct QNorm *qnorm, float x) {
    return (x - qnorm->offset) * qnorm->tscale;
}

float qnorm_restore(const struct QNorm *qnorm, float x) {
    return (x * qnorm->rscale) + qnorm->offset;
}

static void sort(float *begin, float *end) {
    while(begin < --end) {
        int sorted = 1;
        for(float *v = begin; v < end; v++) {
            if(v[1] < v[0]) {
                sorted = 0;
                float tmp = v[1];
                v[1] = v[0];
                v[0] = tmp;
            }
        }
        if(sorted) break;
    }
}
