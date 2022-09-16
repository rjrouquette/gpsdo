//
// Created by robert on 9/13/22.
//

#ifndef GPSDO_FLEXFIS_H
#define GPSDO_FLEXFIS_H

#define DIM_INPUT (4)
#define MAX_NODES (64)
#define NODE_LIM (2*86400)
#define NODE_THR (0.1f)
#define NODE_SEP (0.2f)
#define MAX_AGE (2*86400)
#define MAX_DIST (3.0f)
#define MAX_ERR (1.0f)

void flexfis_init(int count, const float *input, int strideInput, const float *target, int strideTarget);
void flexfis_update(const float *input, float target);
float flexfis_predict(const float *input);

#endif //GPSDO_FLEXFIS_H
