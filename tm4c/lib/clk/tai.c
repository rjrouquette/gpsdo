//
// Created by robert on 4/27/23.
//

#include "../../hw/interrupts.h"
#include "mono.h"
#include "tai.h"
#include "comp.h"
#include "util.h"
#include "../gps.h"

volatile uint64_t clkTaiUtcOffset = 0;

static uint32_t clkTaiRem = 0;
volatile uint64_t clkTaiOffset = 0;
volatile uint64_t clkTaiRef = 0;
volatile int32_t clkTaiRate = 0;


void initClkTai() {
    clkTaiOffset = ((uint64_t) GPS_taiOffset()) << 32;
}

void runClkTai() {
    // advance reference time at roughly 16 Hz to prevent overflow
    uint64_t now = CLK_COMP();
    if((now - clkTaiRef) > (1 << 28))
        CLK_TAI_setTrim(clkTaiRate);
}

uint64_t CLK_TAI() {
    return CLK_TAI_fromMono(CLK_MONO());
}

uint64_t CLK_TAI_fromMono(uint64_t ts) {
    ts = CLK_COMP_fromMono(ts);
    ts += corrValue(clkTaiRate, ts - clkTaiRef, 0);
    ts += clkTaiOffset;
    return ts;
}

void CLK_TAI_setTrim(int32_t comp) {
    // prepare update values
    uint64_t now = CLK_COMP();
    uint64_t offset = corrValue(clkTaiRate, now - clkTaiRef, &clkTaiRem);
    // apply update
    __disable_irq();
    clkTaiRef = now;
    clkTaiRate = comp;
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
    uint64_t now = CLK_COMP();
    __disable_irq();
    ((uint32_t *) &clkTaiOffset)[1] = seconds - ((uint32_t *) &now)[1];
    __enable_irq();
}

void CLK_TAI_adjust(int64_t delta) {
    __disable_irq();
    clkTaiOffset += delta;
    __enable_irq();
}
