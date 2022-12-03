/**
 * Online-Learning Temperature Compensation
 * @author Robert J. Rouquette
 */

#include "tcomp.h"

#define SEQ_LEN (16)
#define ALPHA (0x1p-16f)

static volatile float sequence[SEQ_LEN];
static volatile float regressor[SEQ_LEN];
static volatile float mean[2];
static volatile int init = 1;

static volatile float alpha, beta;

void TCOMP_updateTarget(float target) {
    if(init) {
        init = 0;
        mean[0] = sequence[0];
        mean[1] = target;
    }

    // update means
    mean[0] += ALPHA * (sequence[0] - mean[0]);
    mean[1] += ALPHA * (target - mean[1]);

    float x[SEQ_LEN];
    float error = target - mean[1];
    for(int i = 0; i < SEQ_LEN; i++) {
        x[i] = sequence[i] - mean[0];
        error -= regressor[i] * x[i];
    }
    error *= ALPHA;

    for(int i = 0; i < SEQ_LEN; i++)
        regressor[i] += error * x[i];
}

void TCOMP_updateTemp(float temp) {
    for(int i = SEQ_LEN-1; i > 0; i--)
        sequence[i] = sequence[i-1];
    sequence[0] = temp;
}

float TCOMP_getComp(float *m, float *b) {
    float comp = mean[1];
    beta = 0;
    for(int i = 0; i < SEQ_LEN; i++) {
        comp += regressor[i] * (sequence[i] - mean[0]);
        beta += regressor[i];
    }
    alpha = comp - beta * sequence[0];

    *m = beta;
    *b = alpha;
    return comp;
}
