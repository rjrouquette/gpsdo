//
// Created by robert on 4/27/23.
//

#include "clk.hpp"

#include "mono.h"
#include "util.hpp"
#include "../hw/interrupts.h"
#include "../hw/sys.h"

#define XTL_FREQ (25000000)

static void initClkSys() {
    // Enable external clock
    MOSCCTL.NOXTAL = 0;
    MOSCCTL.OSCRNG = 1;
    MOSCCTL.PWRDN = 0;
    while (!SYSRIS.MOSCPUPRIS) {}

    // Configure OSC and PLL source
    RSCLKCFG.OSCSRC = 0x3;
    RSCLKCFG.PLLSRC = 0x3;

    // Configure PLL
    PLLFREQ1.N = 0;
    PLLFREQ1.Q = 0;
    PLLFREQ0.MINT = (2 * CLK_FREQ) / XTL_FREQ;
    PLLFREQ0.MFRAC = 0;
    RSCLKCFG.PSYSDIV = 1;
    RSCLKCFG.NEWFREQ = 1;
    PLLFREQ0.PLLPWR = 1;

    // Adjust Memory Timing
    MEMTIM0.EWS = 4;
    MEMTIM0.EBCE = 0;
    MEMTIM0.EBCHT = 5;
    MEMTIM0.FWS = 4;
    MEMTIM0.FBCE = 0;
    MEMTIM0.FBCHT = 5;
    // Apply Timing Adjustments
    RSCLKCFG.MEMTIMU = 1;

    // Switch to PLL
    while (!PLLSTAT.LOCK) {}
    RSCLKCFG.USEPLL = 1;
}

// mono.c
void initClkMono();
void initClkEth();
void initClkSync();
// comp.c
void initClkComp();
// tai.c
void initClkTai();

void CLK_initSys() {
    initClkSys();
    initClkMono();
}

void CLK_init() {
    initClkSync();
    initClkEth();
    initClkComp();
    initClkTai();
}


// internal state for CLK_PPS (reduces overhead)
static volatile ClockEvent ppsEvent;
static volatile uint64_t ppsStamp[3];

void CLK_PPS(uint64_t *tsResult) {
    if (
        ppsEvent.timer == clkMonoPpsEvent.timer &&
        ppsEvent.integer == clkMonoPpsEvent.integer
    ) {
        tsResult[0] = ppsStamp[0];
        tsResult[1] = ppsStamp[1];
        tsResult[2] = ppsStamp[2];
        return;
    }

    __disable_irq();
    const_cast<ClockEvent&>(ppsEvent) = const_cast<ClockEvent&>(clkMonoPpsEvent);
    __enable_irq();

    // convert snapshot to timestamps
    uint32_t rem = 0;
    // monotonic clock
    ppsStamp[0] = fromClkMono(ppsEvent.timer, ppsEvent.offset, ppsEvent.integer);
    // frequency compensated clock
    ppsStamp[1] = ppsStamp[0] +
                  corrFracRem(ppsEvent.compRate, ppsStamp[0] - ppsEvent.compRef, rem) +
                  ppsEvent.compOff;
    // TAI disciplined clock
    ppsStamp[2] = ppsStamp[1] +
                  corrFracRem(ppsEvent.taiRate, ppsStamp[1] - ppsEvent.taiRef, rem) +
                  ppsEvent.taiOff;

    // return result
    tsResult[0] = ppsStamp[0];
    tsResult[1] = ppsStamp[1];
    tsResult[2] = ppsStamp[2];
}
