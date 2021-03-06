/**
 * Online-Learning Temperature Compensation
 * 1-D Self-Organizing Map w/ Local Linear Estimators
 * @author Robert J. Rouquette
 */

#include <math.h>
#include <memory.h>
#include <stdlib.h>

#include "tcomp.h"
#include "lib/plot.h"
#include "lib/format.h"
#include "lib/font.h"

#define NODE_CNT (16)
#define TAU_INIT (64)
#define TAU_RUN (2048)
#define DIST_INIT (-1)
#define DIST_RUN (-3)
#define INIT_SOM (1024)
#define INIT_END (2048)

static struct CompNode {
    float meanTemp;
    float meanComp;
    float varTempTemp;
    float varTempComp;
} nodes[NODE_CNT];

static int ndist;
static float tau;
static int initCnt;

void updateNode(struct CompNode *node, float w, float temp, float comp) {
    // learning rate
    const float rate = w / tau;
    // compute and apply delta to means
    float delTemp = rate * (temp - node->meanTemp);
    float delComp = rate * (comp - node->meanComp);
    node->meanTemp += delTemp;
    node->meanComp += delComp;
    if(initCnt < INIT_SOM) return;

    // re-center variables
    temp -= node->meanTemp;
    comp -= node->meanComp;
    // update temperature variance
    node->varTempTemp += rate * ((temp * temp) - node->varTempTemp);
    node->varTempTemp += delTemp * delTemp;
    // update temperature compensation covariance
    node->varTempComp += rate * ((temp * comp) - node->varTempComp);
    node->varTempComp += delTemp * delComp;
}


void TCOMP_init() {
    // clear state
    memset(nodes, 0, sizeof(nodes));
}

void TCOMP_update(float temp, float comp) {
    // first update special case
    if(initCnt == 0) {
        ndist = DIST_INIT;
        tau = TAU_INIT;
        for(int i = 0; i < NODE_CNT; i++) {
            nodes[i].meanTemp = temp + (((float) i) / NODE_CNT);
            nodes[i].meanComp = comp;
        }
    }

    // find best matching node
    int best = 0;
    float min = fabsf(temp - nodes[best].meanTemp);
    for(int i = 1; i < NODE_CNT; i++) {
        float dist = fabsf(temp - nodes[i].meanTemp);
        if(dist < min) {
            best = i;
            min = dist;
        }
    }

    // update nodes
    if(initCnt < INIT_END) {
        if(++initCnt == INIT_END) {
            ndist = DIST_RUN;
            tau = TAU_RUN;
        }
    }
    for(int i = 0; i < NODE_CNT; i++) {
        int dist = abs(i - best);
        updateNode(&nodes[i], ldexpf(1, ndist * dist), temp, comp);
    }
}


void TCOMP_getCoeff(float temp, float *m, float *b) {
    if(initCnt < INIT_END) {
        *m = NAN;
        *b = 0;
    }

    // find best matching node
    int best = 0;
    float min = fabsf(temp - nodes[best].meanTemp);
    for(int i = 1; i < NODE_CNT; i++) {
        float dist = fabsf(temp - nodes[i].meanTemp);
        if(dist < min) {
            best = i;
            min = dist;
        }
    }

    // compute coefficients
    *m = nodes[best].varTempComp / nodes[best].varTempTemp;
    *b = nodes[best].meanComp - (*m) * nodes[best].meanTemp;
}

void TCOMP_plot() {
    char str[32];
    float xmax, xmin, ymax, ymin;
    xmax = xmin = nodes[NODE_CNT/2].meanTemp;
    ymax = ymin = nodes[NODE_CNT/2].meanComp;
    for(int i = 0; i < NODE_CNT; i++) {
        if(nodes[i].meanTemp > xmax) xmax = nodes[i].meanTemp;
        if(nodes[i].meanTemp < xmin) xmin = nodes[i].meanTemp;
        if(nodes[i].meanComp > ymax) ymax = nodes[i].meanComp;
        if(nodes[i].meanComp < ymin) ymin = nodes[i].meanComp;
    }

    if(xmax == xmin) { xmin -= 0.1f; xmax += 0.1f; }
    if(ymax == ymin) { ymin -= 1e-9f; ymax += 1e-9f; }

    float xscale = 159.0f / (xmax - xmin);
    float yscale = 71.0f / (ymin - ymax);

    PLOT_setLine(0, 104, 176, 104, 1);
    PLOT_setRect(0, 105, 176, 182, 3);

    toDec(initCnt, 8, ' ', str);
    FONT_drawText(112, 168, str, FONT_ASCII_16, 0, 3);

    for(int i = 0; i < NODE_CNT; i++) {
        int x = lroundf(xscale * (nodes[i].meanTemp - xmin));
        int y = lroundf(yscale * (nodes[i].meanComp - ymax));
        x += 8;
        y += 108;
        PLOT_setLine(x-2, y, x+2, y, 0);
        PLOT_setLine(x, y-2, x, y+2, 0);
    }

    int px = 0, py = 105;
    for(int x = 0; x < 176; x++) {
        float temp = ((float)(x - 8)) / xscale;
        temp += xmin;
        float m, b;
        TCOMP_getCoeff(temp, &m, &b);
        float comp = (m * temp) + b;
        int y = lroundf(yscale * (comp - ymax));
        y += 108;
        if((x > 0) && (py > 104) && (y > 104) && (py < 183) && (y < 183)) {
            PLOT_setLine(px, py, x, y, 1);
        }
        px = x;
        py = y;
    }
}
