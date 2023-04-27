//
// Created by robert on 4/27/23.
//

#include "../../hw/interrupts.h"
#include "mono.h"
#include "tai.h"
#include "trim.h"
#include "util.h"


static uint32_t clkTaiRem = 0;
volatile uint64_t clkTaiOffset = 0;
volatile uint64_t clkTaiRef = 0;
volatile int32_t clkTaiRate = 0;


uint64_t CLK_TAI() {
    return CLK_TAI_fromMono(CLK_MONO());
}

uint64_t CLK_TAI_PPS() {
    uint64_t ts = CLK_TRIM_PPS();
    ts += corrValue(clkMonoPps.taiRate, ts - clkMonoPps.taiRef, 0);
    return ts + clkMonoPps.taiOff;
}

uint64_t CLK_TAI_fromMono(uint64_t mono) {
    uint64_t trim = CLK_TRIM_fromMono(mono);
    mono += corrValue(clkTaiRate, trim - clkTaiRef, 0);
    mono += clkTaiOffset;
    return mono;
}

void CLK_TAI_setTrim(int32_t tai) {
    // prepare update values
    uint64_t now = CLK_TRIM();
    uint64_t offset = corrValue(clkTaiRate, now - clkTaiRef, &clkTaiRem);
    // apply update
    __disable_irq();
    clkTaiRef = now;
    clkTaiRate = tai;
    clkTaiOffset += offset;
    __enable_irq();
}

int32_t CLK_TAI_getTrim() {
    return clkTaiRate;
}

void CLK_TAI_align(int32_t fraction) {
    __disable_irq();
    clkTaiOffset += fraction;
    __enable_irq();
}

void CLK_TAI_set(uint32_t seconds) {
    uint64_t now = CLK_TRIM();
    __disable_irq();
    ((uint32_t *) &clkTaiOffset)[1] = seconds - ((uint32_t *) &now)[1];
    __enable_irq();
}
