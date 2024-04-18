//
// Created by robert on 4/27/23.
//

#include "clock.hpp"

#include "../delay.hpp"
#include "../hw/emac.h"
#include "../hw/gpio.h"
#include "../hw/sys.h"

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
    PLLFREQ0.MINT = (2 * CLK_FREQ) / MOSC_FREQ;
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

static void initClkEth() {
    // configure PPS output pin
    RCGCGPIO.EN_PORTG = 1;
    delay::cycles(4);
    // unlock GPIO config
    PORTG.LOCK = GPIO_LOCK_KEY;
    PORTG.CR = 0x01u;
    // configure pins
    PORTG.DIR = 0x01u;
    PORTG.DR8R = 0x01u;
    PORTG.PCTL.PMC0 = 0x5;
    PORTG.AFSEL.ALT0 = 1;
    PORTG.DEN = 0x01u;
    // lock GPIO config
    PORTG.CR = 0;
    PORTG.LOCK = 0;

    // disable flash prefetch per errata
    FLASHCONF.FPFOFF = 1;
    // enable MAC and PHY clocks
    RCGCEMAC.EN0 = 1;
    RCGCEPHY.EN0 = 1;
    while (!PREMAC.RDY0) {}
    while (!PREPHY.RDY0) {}

    // disable timer interrupts
    EMAC0.IM.TS = 1;
    // enable PTP clock
    EMAC0.CC.PTPCEN = 1;
    // configure PTP
    EMAC0.TIMSTCTRL.ALLF = 1;
    EMAC0.TIMSTCTRL.DGTLBIN = 1;
    EMAC0.TIMSTCTRL.TSEN = 1;
    // 25MHz = 40ns
    EMAC0.SUBSECINC.SSINC = 40;
    // init timer
    EMAC0.TIMSECU = 0;
    EMAC0.TIMNANOU.VALUE = 0;
    // start timer
    EMAC0.TIMSTCTRL.TSINIT = 1;
    // use PPS free-running mode
    EMAC0.PPSCTRL.TRGMODS0 = 3;
    // start 1 Hz PPS output
    EMAC0.PPSCTRL.PPSEN0 = 0;
    EMAC0.PPSCTRL.PPSCTRL = 0;

    // re-enable flash prefetch per errata
    FLASHCONF.FPFOFF = 0;
}

// capture.cpp
namespace clock::capture {
    void init();
}

// mono.cpp
void initClkMono();

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
    initClkEth();
    initClkComp();
    initClkTai();
}
