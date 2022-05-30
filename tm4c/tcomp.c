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

#define MAX_TAU (4096)
#define NODE_CNT (16)

static struct CompNode {
    float meanTemp;
    float meanComp;
    float varTempTemp;
    float varTempComp;
    float tau;
} nodes[NODE_CNT];

void updateNode(struct CompNode *node, float w, float temp, float comp) {
    // update learning rate
    float tau = node->tau + w;
    if(tau > MAX_TAU) tau = MAX_TAU;
    // compute and apply delta to means
    float delTemp = w * (temp - node->meanTemp) / tau;
    float delComp = w * (comp - node->meanComp) / tau;
    node->meanTemp += delTemp;
    node->meanComp += delComp;

    // update variance and covariance
    if(node->tau > 0) {
        // re-center variables
        temp -= node->meanTemp;
        comp -= node->meanComp;
        // update temperature variance
        node->varTempTemp += w * ((temp * temp) - node->varTempTemp) / (node->tau + w);
        node->varTempTemp += delTemp * delTemp;
        // update temperature compensation covariance
        node->varTempComp += w * ((temp * comp) - node->varTempComp) / (node->tau + w);
        node->varTempComp += delTemp * delComp;
    }
    // update node learning rate
    node->tau = tau;
}


void TCOMP_init() {
    // clear state
    memset(nodes, 0, sizeof(nodes));
}

void TCOMP_update(float temp, float comp) {
    // find best matching node
    int best = NODE_CNT/2;
    float min = fabsf(temp - nodes[best].meanTemp);
    for(int i = 0; i < NODE_CNT; i++) {
        float dist = fabsf(temp - nodes[i].meanTemp);
        if(dist < min) {
            best = i;
            min = dist;
        }
    }

    // update nodes
    for(int i = 0; i < NODE_CNT; i++) {
        int dist = abs(i - best);
        if(dist > 3) continue;
        updateNode(&nodes[i], ldexpf(1, -3 * dist), temp, comp);
    }
}


void TCOMP_getCoeff(float temp, float *m, float *b) {
    // find best matching node
    int best = NODE_CNT/2;
    float min = fabsf(temp - nodes[best].meanTemp);
    for(int i = 0; i < NODE_CNT; i++) {
        if(nodes[i].tau == 0) continue;
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
    float xmax, xmin, ymax, ymin;
    xmax = xmin = nodes[NODE_CNT/2].meanTemp;
    ymax = ymin = nodes[NODE_CNT/2].meanComp;
    for(int i = 0; i < NODE_CNT; i++) {
        if(nodes[i].tau == 0) continue;
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

    for(int i = 0; i < NODE_CNT; i++) {
        if(nodes[i].tau == 0) continue;
        int x = lroundf(xscale * (nodes[i].meanTemp - xmin));
        int y = lroundf(yscale * (nodes[i].meanComp - ymax));
        x += 8;
        y += 108;
        PLOT_setLine(x-3, y-3, x+3, y+3, 0);
        PLOT_setLine(x-3, y+3, x+3, y-3, 0);
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
