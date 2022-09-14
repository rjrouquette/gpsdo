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
    float cov[((DIM_FLEXNODE+1)*DIM_FLEXNODE)/2];
    float reg[DIM_INPUT];
};

static volatile int epoch;
volatile int nodeCount;
struct QNorm norms[DIM_INPUT+1];
struct FlexNode nodes[MAX_NODES];

static void sort(struct FlexDist *begin, struct FlexDist *end);
static int matrix_invert(float *mat, int dim);
static void matrix_mult(const float *matX, int rowsX, int colsX, const float *matY, int rowsY, int colsY, float *matZ);

float flexnode_estimate(const struct FlexNode *node, const float *x) {
    float acc = node->center[0];
    for(int i = 1; i < DIM_FLEXNODE; i++)
        acc += node->reg[i-1] * (x[i] - node->center[i]);
    return acc;
}

void flexnode_updateRegressor(struct FlexNode *node) {
    if(node->n < (DIM_INPUT * 10))
        return;

    // construct regression matrices
    float xx[DIM_INPUT][DIM_INPUT];
    float xy[DIM_INPUT];
    int k = 0;
    for(int i = DIM_INPUT; i > 0; i--) {
        int _i = i - 1;
        for(int j = i; j > 0; j--) {
            int _j = j - 1;
            xx[_i][_j] = node->cov[k];
            xx[_j][_i] = node->cov[k];
            ++k;
        }
        xy[_i] = node->cov[k];
        ++k;
    }
    // compute OLS fit
    if(matrix_invert(xx[0], DIM_INPUT)) {
        matrix_mult(xx[0], DIM_INPUT, DIM_INPUT, xy, DIM_INPUT, 1, node->reg);
    }
}

void flexnode_rescale(struct FlexNode *node, const float *scale, const float *offset) {
    // rescale node center
    for(int i = 0; i < DIM_FLEXNODE; i++) {
        node->center[i] *= scale[i];
        node->center[i] += offset[i];
    }
    // rescale node covariance
    int k = 0;
    for(int i = DIM_FLEXNODE-1; i >= 0; i--) {
        for(int j = i; j >= 0; j--) {
            node->cov[k++] *= scale[i] * scale[j];
        }
    }
    flexnode_updateRegressor(node);
}

void flexnode_updateRules(struct FlexNode *node, const float *x) {
    float delta[DIM_FLEXNODE], diff[DIM_FLEXNODE];

    // record update epoch
    node->touched = epoch;

    // increment sample count
    node->n += 1;
    if(node->n > NODE_LIM)
        node->n = NODE_LIM;

    // update node center
    for(int i = 0; i < DIM_FLEXNODE; i++) {
        delta[i] = (x[i] - node->center[i]) / node->n;
        node->center[i] += delta[i];
        diff[i] = x[i] - node->center[i];
    }
    if(node->n <= 1)
        bzero(delta, sizeof(delta));

    // update node covariance
    int k = 0;
    for(int i = DIM_FLEXNODE-1; i >= 0; i--) {
        for(int j = i; j >= 0; j--) {
            // update cell
            node->cov[k] += ((diff[i] * diff[j]) - node->cov[k]) / node->n;
            node->cov[k] += delta[i] * delta[j];
            ++k;
        }
    }

    flexnode_updateRegressor(node);
}

int flexnode_update(struct FlexNode *node, const float *x) {
//    if(fabsf(x[0] - flexnode_estimate(node, x)) > NODE_THR)
//        return 0;
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
            norms + 0,
            target + ((count-1) * strideTarget),
            count,
            -strideTarget
    );
    for(int i = 0; i < DIM_INPUT; i++) {
        // reverse sample order
        qnorm_init(
                norms + i + 1,
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
    int rescale = qnorm_update(norms+0, &target, 1, 1, offset+0, scale+0);
    for(int i = 0; i < DIM_INPUT; i++)
        rescale |= qnorm_update(norms+i+1, input+i, 1, 1, offset+i+1, scale+i+1);

    // rescale nodes
    if(rescale) {
        for(int i = 0; i < nodeCount; i++)
            flexnode_rescale(nodes+i, scale, offset);
    }

    // transform input vector
    offset[0] = qnorm_transform(norms+0, target);
    for(int i = 0; i < DIM_INPUT; i++)
        offset[i+1] = qnorm_transform(norms+i+1,input[i]);

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
    for(int i = 0; i < nodeCount; i++) {
        if(nodeDist[i].dist > 0.2f * DIM_FLEXNODE)
            break;
        if(flexnode_update(nodes + nodeDist[i].index, offset)) {
            flexfis_pruneNodes();
            return;
        }
    }

    // sanity check
    if(nodeDist[0].dist > 2 * DIM_FLEXNODE) return;

    // sanity check
    float error = offset[0] - flexnode_estimate(nodes + nodeDist[0].index, offset);
    if(fabsf(error) > 0.2f) return;

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




static inline void swap(float **a, float **b) {
    float *temp = *a;
    *a = *b;
    *b = temp;
}

static int matrix_invert(float *mat, const int dim) {
    float *xr[dim], *ir[dim];
    float inv[dim * dim];
    bzero(inv, dim * dim * sizeof(float));
    for(int i = 0; i < dim; i++) {
        xr[i] = mat + (i * dim);
        ir[i] = inv + (i * dim);
        ir[i][i] = 1;
    }

    // reduce to row echelon form
    for(int h = 0; h < dim - 1; h++) {
        // find best pivot
        int imax = h;
        float max = fabsf(xr[h][h]);
        for(int i = h + 1; i < dim; i++) {
            float v = fabsf(xr[i][h]);
            if(v > max) {
                max = v;
                imax = i;
            }
        }

        // stop if matrix is singular
        if(max == 0 || !isfinite(max)) {
            return 0;
        }

        // pop pivot to top
        swap(xr + h, xr + imax);
        swap(ir + h, ir + imax);

        // reduce rows below pivot
        float *ipiv = ir[h];
        float *xpiv = xr[h];
        for(int i = h + 1; i < dim; i++) {
            float *irow = ir[i];
            float *xrow = xr[i];

            float f = xrow[h] / xpiv[h];
            xrow[h] = 0;
            for(int j = h + 1; j < dim; j++) {
                xrow[j] -= xpiv[j] * f;
            }
            for(int j = 0; j < dim; j++) {
                irow[j] -= ipiv[j] * f;
            }
        }
    }

    // reduce upper triangle
    for(int h = dim - 1; h >= 0; h--) {
        const float z = xr[h][h];
        // stop if matrix is singular
        if(z == 0 || !isfinite(z)) {
            return 0;
        }

        float *ipiv = ir[h];
        for(int i = 0; i < h; i++) {
            float f = xr[i][h] / z;
            float *irow = ir[i];
            for(int j = 0; j < dim; j++) {
                irow[j] -= ipiv[j] * f;
            }
        }

        // reduce pivot row
        for(int j = 0; j < dim; j++) {
            ipiv[j] /= z;
        }
    }

    // copy inverse into original matrix
    for(int i = 0; i < dim; i++) {
        float *xrow = mat + (i * dim);
        float *irow = ir[i];
        for(int j = 0; j < dim; j++) {
            xrow[j] = irow[j];
        }
    }
    return 1;
}

static void matrix_mult(
        const float *matX,
        const int rowsX,
        const int colsX,
        const float *matY,
        const int rowsY,
        const int colsY,
        float *matZ
) {
    for(int i = 0; i < rowsX; i++) {
        const float *xrow = matX + (i * colsX);
        float *zrow = matZ + (i * colsY);
        for(int j = 0; j < colsY; j++) {
            const float *yrow = matY;
            float acc = 0;
            for(int k = 0; k < colsX; k++) {
                acc += xrow[k] * yrow[j];
                yrow += colsY;
            }
            zrow[j] = acc;
        }
    }
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
