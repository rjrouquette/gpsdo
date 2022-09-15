//
// Created by robert on 9/13/22.
//

#include <math.h>
#include <strings.h>

#include "qnorm.h"

// neighbor distance lookup table
static float lutZeta[QNORM_SIZE] = {
        1.00000000e+00f,
        1.83156389e-02f,
        3.35462628e-04f,
        6.14421235e-06f,
        1.12535175e-07f,
        2.06115362e-09f,
        3.77513454e-11f,
        6.91440011e-13f,
        1.26641655e-14f,
        2.31952283e-16f,
        4.24835426e-18f,
        7.78113224e-20f,
        1.42516408e-21f,
        2.61027907e-23f,
        4.78089288e-25f,
        8.75651076e-27f,
        1.60381089e-28f,
        2.93748211e-30f,
        5.38018616e-32f,
        9.85415469e-34f,
        1.80485139e-35f,
        3.30570063e-37f,
        6.05460190e-39f,
        1.10893902e-40f,
        2.03109266e-42f,
        3.72007598e-44f
        // single precision drops out at index 26
};


static void updateBin(struct QNormBin *bin, float mu, float x, float w) {
    bin->tau += w;
    bin->mean += (x - mu) * w;
}

static void compBounds(const struct QNorm *qnorm, float *_offset, float *_scale) {
    // compute mean
    float muY = 0;
    for(int i = 0; i < QNORM_SIZE; i++) {
        muY += qnorm->bins[i].mean * qnorm->bins[i].tau;
    }

    // compute covariance
    float acc = 0, m = 0, n = 0;
    for(int i = 0; i < QNORM_SIZE; i++) {
        float x = acc + (0.5f * qnorm->bins[i].tau);
        acc += qnorm->bins[i].tau;

        float dx = x - 0.5f;
        float dy = qnorm->bins[i].mean - muY;

        m += dx * dy * qnorm->bins[i].tau;
        n += dx * dx * qnorm->bins[i].tau;
    }

    // compute bounds
    m /= n;
    *_scale = 0.3f * m;
    *_offset = muY;
}

static void compUpdate(struct QNorm *qnorm, float x, struct QNormBin *acc) {
    int a = 1;
    int b = QNORM_SIZE - 1;
    struct QNormBin *bins = qnorm->bins;

    // locate nearest span
    while(a < b) {
        const int i = (a + b) / 2;
        const float t = bins[i].mean;
        if      (x < t) b = i;
        else if (x > t) a = i+1;
        else            break;
    }
    int i = (a + b) / 2;
    // locate nearest bin
    if(fabsf(x - bins[i].mean) > fabsf(x - bins[i-1].mean)) --i;

    // update bins
    for(int j = 0; j < i; j++)
        updateBin(acc + j, bins[j].mean, x, lutZeta[i-j]);
    for(int j = i; j < QNORM_SIZE; j++)
        updateBin(acc + j, bins[j].mean, x, lutZeta[j-i]);
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

    // compute eta
    const float eta = (qnorm->tau > 0) ? (1.0f / qnorm->tau) : 1.0f;
    for(int i = 0; i < QNORM_SIZE; i++) {
        if(acc[i].tau <= 0) continue;
        qnorm->bins[i].tau += eta * acc[i].tau;
        qnorm->bins[i].mean += eta * acc[i].mean / qnorm->bins[i].tau;
    }

    // update global tau
    qnorm->tau += (float) count;
    if(qnorm->tau > QNORM_LIM) qnorm->tau = QNORM_LIM;

    // normalize bin taus
    float nrm = 0;
    for(int i = 0; i < QNORM_SIZE; i++)
        nrm += qnorm->bins[i].tau;
    nrm = 1.0f / nrm;
    for(int i = 0; i < QNORM_SIZE; i++)
        qnorm->bins[i].tau *= nrm;

    // compute new bounds
    float _offset, _rscale;
    compBounds(qnorm, &_offset, &_rscale);
    float denom = qnorm->rscale + _rscale;

    // only update if there has been a significant change
    if(
        fabsf(_offset - qnorm->offset) / denom > 0.01f ||
        fabsf(_rscale - qnorm->rscale) / denom > 0.01f
    ) {
        // compute scale and offset translation from old bounds to new bounds
        *dScale =  qnorm->rscale / _rscale;
        *dOffset = (qnorm->offset - _offset) / _rscale;
        // set new scale
        qnorm->offset = _offset;
        qnorm->tscale = 1.0f / _rscale;
        qnorm->rscale = _rscale;
        return 1;
    }

    // no change to scaling transform
    *dScale = 1;
    *dOffset = 0;
    return 0;
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
