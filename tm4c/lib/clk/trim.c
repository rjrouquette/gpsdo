//
// Created by robert on 4/26/23.
//

#include "../../hw/interrupts.h"
#include "mono.h"
#include "trim.h"
#include "util.h"

static uint32_t clkTrimRem = 0;
volatile uint64_t clkTrimOffset = 0;
volatile uint64_t clkTrimRef = 0;
volatile int32_t clkTrimRate = 0;


void runClkTrim() {
    // advance reference time at roughly 16 Hz to prevent overflow
    uint64_t now = CLK_MONO();
    if((now - clkTrimRef) > (1 << 28))
        CLK_TRIM_setTrim(clkTrimRate);
}

uint64_t CLK_TRIM() {
    return CLK_TRIM_fromMono(CLK_MONO());
}

uint64_t CLK_TRIM_PPS() {
    uint64_t ts = CLK_MONO_PPS();
    ts += corrValue(clkMonoPps.trimRate, ts - clkMonoPps.trimRef, 0);
    return ts + clkMonoPps.trimOff;
}

uint64_t CLK_TRIM_fromMono(uint64_t ts) {
    ts += corrValue(clkTrimRate, ts - clkTrimRef, 0);
    ts += clkTrimOffset;
    return ts;
}

void CLK_TRIM_setTrim(int32_t trim) {
    // prepare update values
    uint64_t now = CLK_MONO();
    uint64_t offset = corrValue(clkTrimRate, now - clkTrimRef, &clkTrimRem);
    // apply update
    __disable_irq();
    clkTrimRef = now;
    clkTrimRate = trim;
    clkTrimOffset += offset;
    __enable_irq();
}

int32_t CLK_TRIM_getTrim() {
    return clkTrimRate;
}
