//
// Created by robert on 4/29/23.
//

#ifndef GPSDO_PLL_H
#define GPSDO_PLL_H

#include <stdint.h>

#define PLL_MAX_FREQ_TRIM (0x00200000)
#define PLL_MIN_FREQ_TRIM (-0x00200000)

#define PLL_OFFSET_HARD_ALIGN (0x00400000) //  0.976 ms
#define PLL_OFFSET_CORR_BASIS (256e-9f) // unity rate threshold
#define PLL_OFFSET_CORR_MAX (0xfp-4f) // 0.9375
#define PLL_OFFSET_INT_RATE (6) // relative integration rate (log2 of divisor)

#define PLL_DRIFT_INT_RATE (0x1p-8f)

/**
 * Update the TAI offset correction PLL.
 * @param interval the current update interval in log2 seconds
 * @param offset the most recent offset sample
 */
void PLL_updateOffset(int interval, int64_t offset, float rmsOffset);

/**
 * Update the frequency drift correction PLL.
 * @param interval the current update interval in log2 seconds
 * @param drift the most recent drift rate
 */
void PLL_updateDrift(int interval, float drift);

#endif //GPSDO_PLL_H
