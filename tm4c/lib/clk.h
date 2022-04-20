//
// Created by robert on 4/15/22.
//

#ifndef GPSDO_CLK_H
#define GPSDO_CLK_H

#include "stdint.h"
#include "../hw/timer.h"

// initialize system timers
void CLK_init();

// get current epoch
inline uint32_t CLK_MONOTONIC_RAW() { return GPTM0.TAV; }
uint32_t CLK_MONOTONIC_INT();
uint64_t CLK_MONOTONIC();

#endif //GPSDO_CLK_H
