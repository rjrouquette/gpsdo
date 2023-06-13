//
// Created by robert on 4/29/23.
//

#ifndef GPSDO_PLL_H
#define GPSDO_PLL_H

#include <stdint.h>

#define PLL_STATS_ALPHA (0x1p-4f)

#define PLL_MAX_FREQ_TRIM (250e-6f) // 250 ppm

#define PLL_OFFSET_HARD_ALIGN (0x01000000ll) //  39.06 ms
#define PLL_OFFSET_CORR_BASIS (1e-6f) // unity rate threshold
#define PLL_OFFSET_CORR_MAX (0x1p-1f) // dampens oscillation
#define PLL_OFFSET_CORR_MIN (0x1p-4f) // dampens oscillation
#define PLL_OFFSET_INT_RATE (0x1p-5f)


void PLL_init();

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
float PLL_offsetProp();
float PLL_offsetInt();
float PLL_offsetCorr();

// drift stats
float PLL_driftLast();
float PLL_driftMean();
float PLL_driftRms();
float PLL_driftStdDev();
float PLL_driftCorr();
float PLL_driftFreq();

#endif //GPSDO_PLL_H
