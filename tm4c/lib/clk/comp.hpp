//
// Created by robert on 4/26/23.
//

#pragma once

#include <cstdint>

extern volatile uint64_t clkCompOffset;
extern volatile uint64_t clkCompRef;
extern volatile int32_t clkCompRate;

/**
 * Returns the current value of the compensated system clock
 * @return 64-bit fixed-point format (32.32)
 */
uint64_t CLK_COMP();

/**
 * Translate monotonic timestamp to compensated timestamp
 * @param ts timestamp to translate (32.32)
 * @return 64-bit fixed-point format (32.32)
 */
uint64_t CLK_COMP_fromMono(uint64_t ts);

/**
 * Compensate system clock frequency
 * @param comp new comp rate (0.31 fixed-point)
 */
void CLK_COMP_setComp(int32_t comp);

/**
 * Get current compensation rate for system clock frequency
 * @return current compensation rate (0.31 fixed-point)
 */
int32_t CLK_COMP_getComp();
