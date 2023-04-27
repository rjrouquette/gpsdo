//
// Created by robert on 4/15/22.
//

#include "clk.h"
#include "clk/mono.h"
#include "clk/trim.h"
#include "../hw/interrupts.h"

uint64_t taiOffset;

void CLK_init() {
    CLK_MONO_init();
}

uint64_t CLK_TAI() {
    return CLK_TRIM() + taiOffset;
}

uint64_t CLK_TAI_PPS() {
    return CLK_TRIM_PPS() + clkMonoPps.taiOffset;
}

void CLK_TAI_align(int32_t fraction) {
    __disable_irq();
    taiOffset += fraction;
    __enable_irq();
}

void CLK_TAI_set(uint32_t seconds) {
    uint64_t nowMono = CLK_MONO();
    ((uint32_t *) &taiOffset)[1] = seconds - ((uint32_t *) &nowMono)[1];
}

float CLK_TAI_trim(float trim) {
    return 0;
}
