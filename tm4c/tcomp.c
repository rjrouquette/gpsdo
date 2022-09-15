/**
 * Online-Learning Temperature Compensation
 * @author Robert J. Rouquette
 */

#include <math.h>
#include <memory.h>

#include "flexfis.h"
#include "qnorm.h"
#include "tcomp.h"
#include "lib/format.h"
#include "lib/font.h"

#define BOOT_LEN (300)

extern struct QNorm norms[DIM_INPUT+1];

static volatile int initCnt;
static volatile float bootStrap[BOOT_LEN * (DIM_INPUT+1)];

static volatile float alpha, beta;
static volatile float _temp, _targ;

void TCOMP_init() {
    // clear state
    initCnt = 0;
}

void TCOMP_update(const float *temp, const float target) {
    if(initCnt < BOOT_LEN) {
        float *row = (float *) bootStrap;
        row += initCnt * (DIM_INPUT+1);
        row[0] = target;
        for(int i = 0; i < DIM_INPUT; i++)
            row[i+i] = temp[i];
        if(++initCnt >= BOOT_LEN) {
            flexfis_init(BOOT_LEN, (float*)bootStrap+1, DIM_INPUT+1, (float*)bootStrap, DIM_INPUT+1);
        }
    }
    else {
        flexfis_update(temp, target);
        _temp = qnorm_transform(&norms[1], temp[0]);
        _targ = qnorm_transform(&norms[0], target);
    }
}


void TCOMP_getCoeff(const float *temp, float *m, float *b) {
    *m = beta = 0;
    *b = alpha = flexfis_predict(temp);
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

    len = fmtFloat(norms[0].offset, 12, 4, str);
    str[len] = 0;
    FONT_drawText(0, 128, str, FONT_ASCII_16, 0, 3);

    len = fmtFloat(norms[0].rscale, 12, 4, str);
    str[len] = 0;
    FONT_drawText(0, 144, str, FONT_ASCII_16, 0, 3);

    len = fmtFloat(norms[1].offset, 12, 4, str);
    str[len] = 0;
    FONT_drawText(0, 160, str, FONT_ASCII_16, 0, 3);

    len = fmtFloat(norms[1].rscale, 12, 4, str);
    str[len] = 0;
    FONT_drawText(0, 176, str, FONT_ASCII_16, 0, 3);
}
