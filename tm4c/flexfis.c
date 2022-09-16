/**
 * FlexFIS: Flexible Fuzzy Inference Systems
 *
 * FlexFIS is an extension of eVQ (Evolving Vector Quantization) that produces
 * a regessor toplogoy which supports on-line learning and is robust against
 * systemic changes and process deviations.
 *
 * DEVELOPER: Robert J. Rouquette
 * REFERENCE:
 *      TITLE:  Evolving Fuzzy Systems
 *      AUTHOR: Edwin Lughofer
 *      YEAR:   2011
 *      ISBN:   978-3-642-18086-6
 */

#include <math.h>
#include <strings.h>
#include <string.h>
#include "flexfis.h"
#include "qnorm.h"

#define DIM_FLEXNODE (DIM_INPUT+1)

struct FlexDist {
    int index;
    float dist;
};

struct FlexNode {
    int touched;
    float n;
    float center[DIM_FLEXNODE];
    float reg[DIM_INPUT];
};

static volatile int epoch;
volatile int nodeCount;
struct QNorm norms[DIM_INPUT+1];
struct FlexNode nodes[MAX_NODES];

static void sort(struct FlexDist *begin, struct FlexDist *end);

float flexnode_estimate(const struct FlexNode *node, const float *x) {
    float acc = node->center[0];
    for(int i = 1; i < DIM_FLEXNODE; i++)
        acc += node->reg[i-1] * (x[i] - node->center[i]);
    return acc;
}

void flexnode_rescale(struct FlexNode *node, const float *scale, const float *offset) {
    // rescale node center
    for(int i = 0; i < DIM_FLEXNODE; i++) {
        node->center[i] *= scale[i];
        node->center[i] += offset[i];
    }
    // rescale node regressor
    for(int i = 0; i < DIM_INPUT; i++) {
        node->reg[i] *= scale[0];
        node->reg[i] /= scale[i+1];
    }
}

void flexnode_updateRules(struct FlexNode *node, const float *x) {
    // record update epoch
    node->touched = epoch;

    // increment sample count
    node->n += 1;
    if(node->n > NODE_LIM)
        node->n = NODE_LIM;

    // update node center
    float diff[DIM_FLEXNODE];
    for(int i = 0; i < DIM_FLEXNODE; i++) {
        node->center[i] += (x[i] - node->center[i]) / node->n;
        diff[i] = x[i] - node->center[i];
    }

    // update node regressor
    float error = diff[0];
    for(int i = 0; i < DIM_INPUT; i++)
        error -= node->reg[i] * diff[i+1];
    error /= 1024;

    for(int i = 0; i < DIM_INPUT; i++)
        node->reg[i] += error * diff[i+1];
}

int flexnode_update(struct FlexNode *node, const float *x) {
    if (fabsf(x[0] - flexnode_estimate(node, x)) > NODE_THR)
        return 0;
    flexnode_updateRules(node, x);
    return 1;
}



float flexfis_predDist(const float *a, const float *b) {
    float acc = 0, diff;
    for(int i = 1; i < DIM_FLEXNODE; i++) {
        diff = a[i] - b[i];
        acc += diff * diff;
    }
    return acc;
}

float flexfis_nodeDist(const float *a, const float *b) {
    float acc = 0, diff;
    for(int i = 0; i < DIM_FLEXNODE; i++) {
        diff = a[i] - b[i];
        acc += diff * diff;
    }
    return acc;
}

void flexfis_pruneNodes() {
    for(int i = 0; i < nodeCount;) {
        // check age of node
        if((epoch - nodes[i].touched) <= MAX_AGE) {
            ++i;
            continue;
        }
        // prune old node
        memcpy(nodes+i, nodes+(--nodeCount), sizeof(struct FlexNode));
    }
}

void flexfis_freeNode() {
    int oldest = 0;
    int idx = 0;
    for(int i = 0; i < nodeCount; i++) {
        int age = epoch - nodes[i].touched;
        if(age > oldest) {
            oldest = age;
            idx = i;
        }
    }
    // prune old node
    memcpy(nodes+idx, nodes+(--nodeCount), sizeof(struct FlexNode));
}

void flexfis_init(int count, const float *input, int strideInput, const float *target, int strideTarget) {
    // initialize normalizers
    // reverse sample order
    qnorm_init(
            &norms[0],
            target + ((count-1) * strideTarget),
            count,
            -strideTarget
    );
    for(int i = 0; i < DIM_INPUT; i++) {
        // reverse sample order
        qnorm_init(
                &norms[i+1],
                input + i + ((count-1) * strideInput),
                count,
                -strideInput
        );
    }

    // initialize nodes
    bzero(nodes, sizeof(nodes));
    epoch = 0;
    nodeCount = 0;
    for(int i = 0; i < count; i++) {
        flexfis_update(input, target[0]);
        input += strideInput;
        target += strideTarget;
    }
}

void flexfis_update(const float *input, const float target) {
    ++epoch;

    // update normalizers
    float offset[DIM_FLEXNODE], scale[DIM_FLEXNODE];
    int rescale = qnorm_update(&norms[0], &target, 1, 1, offset+0, scale+0);
    for(int i = 0; i < DIM_INPUT; i++)
        rescale |= qnorm_update(&norms[i+1], input+i, 1, 1, offset+i+1, scale+i+1);

    // rescale nodes
    if(rescale) {
        for(int i = 0; i < nodeCount; i++)
            flexnode_rescale(nodes+i, scale, offset);
    }

    // transform input vector
    offset[0] = qnorm_transform(&norms[0], target);
    for(int i = 0; i < DIM_INPUT; i++)
        offset[i+1] = qnorm_transform(&norms[i+1],input[i]);

    // first node special case
    if(nodeCount < 1) {
        // create new node
        struct FlexNode *node = nodes + (nodeCount++);
        bzero(node, sizeof(struct FlexNode));
        flexnode_updateRules(node, offset);
        return;
    }

    // compute node proximity to sample
    struct FlexDist nodeDist[nodeCount];
    for(int i = 0; i < nodeCount; i++) {
        nodeDist[i].dist = flexfis_nodeDist(offset, nodes[i].center);
        nodeDist[i].index = i;
    }
    sort(nodeDist, nodeDist + nodeCount);

    // search for best match
    int isDone = 0;
    for(int i = 0; i < nodeCount; i++) {
        if(nodeDist[i].dist > NODE_SEP * DIM_FLEXNODE) {
            if(fabsf(offset[0] - flexnode_estimate(nodes + nodeDist[i].index, offset)) < NODE_THR) {
                flexnode_update(nodes + nodeDist[i].index, offset);
                flexfis_pruneNodes();
                return;
            }
        }
        else if(flexnode_update(nodes + nodeDist[i].index, offset)) {
            flexfis_pruneNodes();
            return;
        }
    }

    // sanity check
    if(nodeDist[0].dist > MAX_DIST * DIM_FLEXNODE) return;

    // sanity check
    float error = offset[0] - flexnode_estimate(nodes + nodeDist[0].index, offset);
    if(fabsf(error) > MAX_ERR) return;

    // discard oldest node if required
    if(nodeCount >= MAX_NODES) {
        flexfis_freeNode();
    }

    // create new node
    struct FlexNode *node = nodes + (nodeCount++);
    bzero(node, sizeof(struct FlexNode));
    flexnode_updateRules(node, offset);
    flexfis_pruneNodes();
}

float flexfis_predict(const float *input) {
    float offset[DIM_FLEXNODE];

    // transform input vector
    offset[0] = NAN;
    for(int i = 0; i < DIM_INPUT; i++)
        offset[i+1] = qnorm_transform(norms+i+1, input[i]);

    if(nodeCount < 1) return NAN;
    if(nodeCount < 2) {
        float estimate = flexnode_estimate(nodes, offset);
        return qnorm_restore(norms, estimate);
    }

    struct FlexDist nodeDist[nodeCount];
    for(int i = 0; i < nodeCount; i++) {
        nodeDist[i].dist = flexfis_predDist(offset, nodes[i].center);
        nodeDist[i].index = i;
    }
    sort(nodeDist, nodeDist + nodeCount);

    float estimate = flexnode_estimate(nodes + nodeDist[0].index, offset);
    return qnorm_restore(norms, estimate);
}



static void sort(struct FlexDist *begin, struct FlexDist *end) {
    while(begin < --end) {
        int sorted = 1;
        for(struct FlexDist *v = begin; v < end; v++) {
            if(v[1].dist < v[0].dist) {
                sorted = 0;
                struct FlexDist tmp = v[1];
                v[1] = v[0];
                v[0] = tmp;
            }
        }
        if(sorted) break;
    }
}
