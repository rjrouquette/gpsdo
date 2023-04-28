//
// Created by robert on 4/26/23.
//

#include "../../hw/interrupts.h"
#include "mono.h"
#include "comp.h"
#include "util.h"

static uint32_t clkCompRem = 0;
volatile uint64_t clkCompOffset = 0;
volatile uint64_t clkCompRef = 0;
volatile int32_t clkCompRate = 0;


void runClkComp() {
    // advance reference time at roughly 16 Hz to prevent overflow
    uint64_t now = CLK_MONO();
    if((now - clkCompRef) > (1 << 28))
        CLK_COMP_setComp(clkCompRate);
}

uint64_t CLK_COMP() {
    return CLK_COMP_fromMono(CLK_MONO());
}

uint64_t CLK_COMP_fromMono(uint64_t ts) {
    ts += corrValue(clkCompRate, ts - clkCompRef, 0);
    ts += clkCompOffset;
    return ts;
}

void CLK_COMP_setComp(int32_t comp) {
    // prepare update values
    uint64_t now = CLK_MONO();
    uint64_t offset = corrValue(clkCompRate, now - clkCompRef, &clkCompRem);
    // apply update
    __disable_irq();
    clkCompRef = now;
    clkCompRate = comp;
    clkCompOffset += offset;
    __enable_irq();
}

int32_t CLK_COMP_getComp() {
    return clkCompRate;
}
