/**
 * Online-Learning Temperature Compensation
 * @author Robert J. Rouquette
 */

#include <math.h>
#include <memory.h>

#include "flexfis.h"
#include "tcomp.h"
#include "lib/format.h"
#include "lib/font.h"

#define BOOT_LEN (600)

static volatile int initCnt;
static volatile float bootStrap[BOOT_LEN * (DIM_INPUT+1)];

static volatile float alpha, beta;

void TCOMP_init() {
    // clear state
    initCnt = 0;
    flexfis_init(0, NULL, 0, NULL, 0);
}

void TCOMP_update(const float *temp, float comp) {
    if(initCnt < BOOT_LEN) {
        float *row = (float *) bootStrap + (initCnt++)*((DIM_INPUT+1));
        row[0] = comp;
        for(int i = 0; i < DIM_INPUT; i++)
            row[i+i] = temp[i];
        if(initCnt >= BOOT_LEN) {
            flexfis_init(BOOT_LEN, (float*)bootStrap + 1, DIM_INPUT+1, (float*)bootStrap, DIM_INPUT+1);
        }
    }
    else {
        flexfis_update(temp, comp);
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
}
