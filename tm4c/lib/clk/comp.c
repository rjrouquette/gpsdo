//
// Created by robert on 4/26/23.
//

#include "../../hw/interrupts.h"
#include "../../hw/timer.h"
#include "mono.h"
#include "comp.h"
#include "util.h"

#define MIN_UPDT_INTV (CLK_FREQ / 4) // 4 Hz

static volatile uint32_t clkCompUpdate = 0;
static volatile uint32_t clkCompRem = 0;
volatile uint64_t clkCompOffset = 0;
volatile uint64_t clkCompRef = 0;
volatile int32_t clkCompRate = 0;


void runClkComp() {
    // periodically update alignment to prevent numerical overflow
    if((GPTM0.TAV.raw - clkCompUpdate) > 0) {
        clkCompUpdate += MIN_UPDT_INTV;
        CLK_COMP_setComp(clkCompRate);
    }
}

uint64_t CLK_COMP() {
    return CLK_COMP_fromMono(CLK_MONO());
}

uint64_t CLK_COMP_fromMono(uint64_t ts) {
    ts += corrValue(clkCompRate, (int64_t) (ts - clkCompRef));
    ts += clkCompOffset;
    return ts;
}

void CLK_COMP_setComp(int32_t comp) {
    // prepare update values
    uint64_t now = CLK_MONO();
    uint64_t offset = corrFrac(clkCompRate, (uint32_t) (now - clkCompRef), &clkCompRem);

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
