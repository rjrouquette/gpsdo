/**
 * Online-Learning Temperature Compensation
 * 1-D Self-Organizing Map w/ Local Linear Estimators
 * @author Robert J. Rouquette
 */

#include <math.h>
#include <memory.h>
#include <stdlib.h>

#include "tcomp.h"

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
    int best = 0;
    float min = fabsf(temp - nodes[0].meanTemp);
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
    int best = 0;
    float min = fabsf(temp - nodes[0].meanTemp);
    for(int i = 0; i < NODE_CNT; i++) {
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
