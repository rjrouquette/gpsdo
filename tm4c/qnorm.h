//
// Created by robert on 9/13/22.
//

#ifndef GPSDO_QNORM_H
#define GPSDO_QNORM_H

#define QNORM_SIZE (32)
#define QNORM_LIM (5*86400)

struct QNormBin {
    float mean;
    float tau;
};

struct QNorm {
    struct QNormBin bins[QNORM_SIZE];

    float eta;
    float tau;
    float lower;
    float upper;
    float range;
    float tscale;
    float rscale;
};

void qnorm_clear(struct QNorm *qnorm);
void qnorm_init(struct QNorm *qnorm, const float *data, int count, int stride);
int qnorm_update(struct QNorm *qnorm, const float *x, int count, int stride, float *dOffset, float *dScale);
float qnorm_transform(const struct QNorm *qnorm, float value);
float qnorm_restore(const struct QNorm *qnorm, float value);

#endif //GPSDO_QNORM_H
