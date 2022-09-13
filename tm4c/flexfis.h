//
// Created by robert on 9/13/22.
//

#ifndef GPSDO_FLEXFIS_H
#define GPSDO_FLEXFIS_H

#define DIM_INPUT (1)
#define MAX_NODES (64)
#define NODE_LIM (5*86400)
#define NODE_THR (0.05f)
#define PRED_PSY (1e-12f)
#define MAX_AGE (5*86400)

void flexfis_init(int count, const float *input, int strideInput, const float *target, int strideTarget);
void flexfis_update(const float *input, float target);
float flexfis_predict(const float *input);

#endif //GPSDO_FLEXFIS_H
