//
// Created by robert on 4/29/23.
//

#ifndef GPSDO_PLL_H
#define GPSDO_PLL_H

#include <stdint.h>

#define PLL_STATS_ALPHA (0x1p-3f)

#define PLL_MAX_FREQ_TRIM (0x00200000)
#define PLL_MIN_FREQ_TRIM (-0x00200000)

#define PLL_OFFSET_HARD_ALIGN (0x01000000) //  3.906 ms
#define PLL_OFFSET_CORR_BASIS (256e-9f) // unity rate threshold
#define PLL_OFFSET_CORR_MAX (0xfp-4f) // 0.9375
#define PLL_OFFSET_CORR_CAP (10e-6f) // do not correct by more than 10 ppm in a single update
#define PLL_OFFSET_INT_RATE (6) // relative integration rate (log2 of divisor)

#define PLL_DRIFT_INT_RATE (0x1p-8f)

void PLL_init();
void PLL_run();

/**
 * Update the TAI offset correction PLL.
 * @param interval the current update interval in log2 seconds
 * @param offset the most recent offset sample
 */
void PLL_updateOffset(int interval, int64_t offset);

/**
 * Update the frequency drift correction PLL.
 * @param interval the current update interval in log2 seconds
 * @param drift the most recent drift rate
 */
void PLL_updateDrift(int interval, float drift);

/**
 * Write human readable status to buffer
 * @param buffer destination
 * @return number of bytes written
 */
unsigned PLL_status(char *buffer);

// offset stats
float PLL_offsetLast();
float PLL_offsetMean();
float PLL_offsetRms();
float PLL_offsetStdDev();

// drift stats
float PLL_driftLast();
float PLL_driftMean();
float PLL_driftRms();
float PLL_driftStdDev();

#endif //GPSDO_PLL_H
