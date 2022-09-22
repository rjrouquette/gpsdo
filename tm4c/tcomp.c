/**
 * Online-Learning Temperature Compensation
 * @author Robert J. Rouquette
 */

#include <memory.h>

#include "tcomp.h"
#include "lib/format.h"
#include "lib/font.h"

#define SEQ_LEN (64)
#define TAU_LIM (4096)

static volatile float sequence[SEQ_LEN];
static volatile float regressor[SEQ_LEN];
static volatile float mean[2];
static volatile float tau;

static volatile float alpha, beta;

void TCOMP_updateTarget(float target) {
    // update means
    tau += 1.0f;
    if(tau > TAU_LIM) tau = TAU_LIM;
    mean[0] += (sequence[0] - mean[0]) / tau;
    mean[1] += (target - mean[1]) / tau;

    float x[SEQ_LEN];
    float error = target - mean[1];
    for(int i = 0; i < SEQ_LEN; i++) {
        x[i] = sequence[i] - mean[0];
        error -= regressor[i] * x[i];
    }
    error /= TAU_LIM;//tau[1];

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

void TCOMP_plot() {
    char str[32];

    int len;
    len = fmtFloat(alpha * 1e6f, 12, 4, str);
    strcpy(str+len, " ppm");
    FONT_drawText(0, 96, str, FONT_ASCII_16, 0, 3);

    len = fmtFloat(beta * 1e6f, 12, 4, str);
    strcpy(str+len, " ppm/C");
    FONT_drawText(0, 112, str, FONT_ASCII_16, 0, 3);
}
