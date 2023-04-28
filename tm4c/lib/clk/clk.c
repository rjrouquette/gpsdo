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
void initClkRxTx();
void initClkSync();

void CLK_init() {
    initClkSys();
    initClkMono();
    initClkRxTx();
    initClkSync();
}


// trim.c
void runClkTrim();
// tai.c
void runClkTai();

void CLK_run() {
    runClkTrim();
    runClkTai();
}

void CLK_PPS(uint64_t *tsResult) {
    struct ClockEvent ppsEvent;
    __disable_irq();
    ppsEvent = clkMonoPpsEvent;
    __enable_irq();

    tsResult[0] = fromClkMono(ppsEvent.timer, ppsEvent.offset, ppsEvent.integer);
    tsResult[1] = tsResult[0] + corrValue(ppsEvent.trimRate, tsResult[0] - ppsEvent.trimRef, 0) + ppsEvent.trimOff;
    tsResult[2] = tsResult[1] + corrValue(ppsEvent.taiRate, tsResult[1] - ppsEvent.taiRef, 0) + ppsEvent.taiOff;
}
