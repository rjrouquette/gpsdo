//
// Created by robert on 4/27/23.
//

#include "../hw/sys.h"
#include "clk.h"
#include "mono.h"
#include "util.h"
#include "../../hw/interrupts.h"

static void initClkSys() {
    // Enable external clock
    MOSCCTL.NOXTAL = 0;
    MOSCCTL.OSCRNG = 1;
    MOSCCTL.PWRDN = 0;
    while(!SYSRIS.MOSCPUPRIS);

    // Configure OSC and PLL source
    RSCLKCFG.OSCSRC = 0x3;
    RSCLKCFG.PLLSRC = 0x3;

    // Configure PLL
    PLLFREQ1.N = 0;
    PLLFREQ1.Q = 0;
    PLLFREQ0.MINT = 15;
    PLLFREQ0.MFRAC = 0;
    PLLFREQ0.PLLPWR = 1;
    RSCLKCFG.NEWFREQ = 1;
    while(!PLLSTAT.LOCK);
    // Update Memory Timing
    MEMTIM0.EWS = 5;
    MEMTIM0.EBCE = 0;
    MEMTIM0.EBCHT = 6;
    MEMTIM0.FWS = 5;
    MEMTIM0.FBCE = 0;
    MEMTIM0.FBCHT = 6;
    // Apply Changes
    RSCLKCFG.MEMTIMU = 1;
    // Switch to PLL
    RSCLKCFG.PSYSDIV = 2;
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

void CLK_init() {
    initClkSys();
    initClkMono();
    initClkSync();
    initClkEth();
    initClkComp();
    initClkTai();
}


// internal state for CLK_PPS (reduces overhead)
static volatile struct ClockEvent ppsEvent;
static volatile uint64_t ppsStamp[3];

void CLK_PPS(uint64_t *tsResult) {
    if(
            ppsEvent.timer == clkMonoPpsEvent.timer &&
            ppsEvent.integer == clkMonoPpsEvent.integer
    ) {
        tsResult[0] = ppsStamp[0];
        tsResult[1] = ppsStamp[1];
        tsResult[2] = ppsStamp[2];
        return;
    }

    __disable_irq();
    ppsEvent = clkMonoPpsEvent;
    __enable_irq();

    // convert snapshot to timestamps
    uint32_t rem = 0;
    ppsStamp[0] = fromClkMono(ppsEvent.timer, ppsEvent.offset, ppsEvent.integer);
    ppsStamp[1] = ppsStamp[0] + corrFrac(ppsEvent.compRate, ppsStamp[0] - ppsEvent.compRef, &rem) + ppsEvent.compOff;
    ppsStamp[2] = ppsStamp[1] + corrFrac(ppsEvent.taiRate, ppsStamp[1] - ppsEvent.taiRef, &rem) + ppsEvent.taiOff;

    // return result
    tsResult[0] = ppsStamp[0];
    tsResult[1] = ppsStamp[1];
    tsResult[2] = ppsStamp[2];
}
