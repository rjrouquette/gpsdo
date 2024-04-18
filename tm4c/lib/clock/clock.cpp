//
// Created by robert on 4/27/23.
//

#include "clock.hpp"

#include "mono.hpp"
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

// capture.cpp
namespace clock::capture {
    void init();
}

// mono.cpp
void initClkMono();
void initClkEth();
void initClkSync();

// comp.cpp
void initClkComp();

// tai.cpp
void initClkTai();

void clock::initSystem() {
    initClkSys();
    initClkMono();
}

void clock::init() {
    capture::init();
    initClkSync();
    initClkEth();
    initClkComp();
    initClkTai();
}
