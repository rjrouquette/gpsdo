/**
 * Online SOM Regressor
 */

#ifndef GPSDO_OSR_H
#define GPSDO_OSR_H

#define DIM_COORD (1)
#define DIM_INPUT (16)
#define DIM_NODES (16)

#define OSR_ALPHA (0x1p-12f)
#define OSR_SIGMA (2.0f)

struct OSR_Node {
    float coord[DIM_COORD];
    float center[DIM_INPUT+1];
    float regressor[DIM_INPUT];
};

void OSR_init(int count, const float *input, int strideInput, const float *target, int strideTarget);
void OSR_update(const float *input, float target);
float OSR_predict(const float *input);

#endif //GPSDO_OSR_H
