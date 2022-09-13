/**
 * Online-Learning Temperature Compensation
 * 1-D Self-Organizing Map w/ Local Linear Estimators
 * @author Robert J. Rouquette
 */

#include <math.h>
#include <memory.h>
#include <stdlib.h>

#include "flexfis.h"
#include "tcomp.h"
#include "lib/format.h"
#include "lib/font.h"

#define BOOT_LEN (1024)

static int initCnt;
static float bootStrap[BOOT_LEN][DIM_INPUT+1];

static float alpha, beta;

void TCOMP_init() {
    // clear state
    flexfis_init(0, NULL, 0, NULL, 0);
}

void TCOMP_update(const float *temp, float comp) {
    if(initCnt < BOOT_LEN) {
        float *row = bootStrap[initCnt++];
        row[0] = comp;
        for(int i = 0; i < DIM_INPUT; i++)
            row[i+i] = temp[i];
        if(initCnt >= BOOT_LEN) {
            flexfis_init(BOOT_LEN, bootStrap[0] + 1, DIM_INPUT+1, bootStrap[0], DIM_INPUT+1);
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
