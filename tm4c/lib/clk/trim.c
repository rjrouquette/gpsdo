//
// Created by robert on 4/26/23.
//

#include "trim.h"
#include "mono.h"
#include "../../hw/interrupts.h"

static volatile uint64_t clkTrimOffsetRem = 0;
volatile uint64_t clkTrimOffset = 0;
volatile uint64_t clkTrimRef = 0;
volatile int32_t clkTrimRate = 0;

static uint64_t corrValue(uint32_t rate, uint64_t delta, uint64_t *rem) {
    int neg = 0;
    if(delta >> 63) {
        delta = -delta;
        neg = 1;
    }
    if(rate >> 31 & 1) {
        rate = -rate;
        neg ^= 1;
    }
    delta *= rate;
    if(neg) delta = -delta;
    if(rem) {
        delta += *rem;
        *rem = delta - (delta & (-(1ull << 32)));
    }
    return ((int64_t) delta) >> 32;
}

uint64_t CLK_TRIM() {
    return CLK_TRIM_fromMono(CLK_MONO());
}

uint64_t CLK_TRIM_PPS() {
    uint64_t ts = CLK_MONO_PPS();
    ts += corrValue(clkMonoPps.trimRate, ts - clkMonoPps.trimRef, 0);
    return ts + clkMonoPps.trimOff;
}

uint64_t CLK_TRIM_fromMono(uint64_t mono) {
    mono += corrValue(clkTrimRate, mono - clkTrimRef, 0);
    mono += clkTrimOffset;
    return mono;
}

void CLK_TRIM_set(int32_t trim) {
    // prepare update values
    uint64_t now = CLK_MONO();
    uint64_t offset = corrValue(clkTrimRate, now - clkTrimRef, (uint64_t *) &clkTrimOffsetRem);
    // apply update
    __disable_irq();
    clkTrimRef = now;
    clkTrimRate = trim;
    clkTrimOffset += offset;
    __enable_irq();
}

int32_t CLK_TRIM_get() {
    return clkTrimRate;
}
