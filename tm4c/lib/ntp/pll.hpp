//
// Created by robert on 4/29/23.
//

#pragma once

#include <cstdint>


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
