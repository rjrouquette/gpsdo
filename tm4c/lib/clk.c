//
// Created by robert on 4/15/22.
//

#include "../hw/sys.h"
#include "../hw/timer.h"
#include "clk.h"
#include "delay.h"
#include "../hw/emac.h"

#define USE_XTAL 1
#define CLK_FREQ (125000000)
#define CLK_MOD (32u*125000000u)

static volatile uint32_t cntMonotonic = 0;

void CLK_init() {
    // Enable external clock
    MOSCCTL.NOXTAL = 0;
#ifdef USE_XTAL
    MOSCCTL.OSCRNG = 1;
    MOSCCTL.PWRDN = 0;
    while(!SYSRIS.MOSCPUPRIS);
#endif
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

    // Configure SysTick Timer
    STRELOAD.RELOAD = 0xFFFFFF;
    STCTRL.CLK_SRC = 1;
    STCTRL.INTEN = 0;
    STCTRL.ENABLE = 1;

    // Enable Timer 0
    RCGCTIMER.EN_GPTM0 = 1;
    delay_cycles_4();
    // Configure Timer 0
    GPTM0.TAILR = CLK_MOD-1;
    GPTM0.TAMATCHR = CLK_FREQ;
    GPTM0.TAMR.MR = 0x2;
    GPTM0.TAMR.CDIR = 1;
    GPTM0.TAMR.CINTD = 0;
    GPTM0.IMR.TAM = 1;
    GPTM0.TAMR.MIE = 1;
    // start timer
    GPTM0.CTL.TAEN = 1;
}

// second boundary comparison
void ISR_Timer0A() {
    // clear interrupt flag
    GPTM0.ICR.TAM = 1;
    // increment counter
    ++cntMonotonic;
    // set next second boundary
    GPTM0.TAMATCHR += CLK_FREQ;
    if (GPTM0.TAMATCHR >= CLK_MOD)
        GPTM0.TAMATCHR = 0;
}

// return integer part of current time
uint32_t CLK_MONOTONIC_INT() {
    return cntMonotonic;
}

// return current time as 32.32 fixed point value
uint64_t CLK_MONOTONIC() {
    // capture current time
    register uint32_t snapI = cntMonotonic;
    register uint32_t snapF = GPTM0.TAV.raw;

    // scratch structure
    register union {
        struct {
            uint32_t fpart;
            uint32_t ipart;
        };
        uint64_t full;
    } scratch;

    // initialize scratch
    scratch.fpart = snapF;
    scratch.ipart = 0;

    // compute fractional adjustment
    scratch.full *= 45443074;
    scratch.fpart = scratch.ipart;
    scratch.ipart = 0;

    // compute final result
    scratch.full += snapF;
    scratch.full *= 34;

    // merge with full integer count
    scratch.ipart ^= snapI;
    scratch.ipart &= 1;
    scratch.ipart += snapI;
    return scratch.full;
}

uint32_t CLK_GPS_INT() {
    return EMAC0.TIMSEC;
}

uint64_t CLK_GPS() {
    // result structure
    register union {
        struct {
            uint32_t fpart;
            uint32_t ipart;
        };
        uint64_t full;
    } a, b;

    // get current GPS time
    a.ipart = EMAC0.TIMSEC;
    a.fpart = EMAC0.TIMNANO;
    b.ipart = EMAC0.TIMSEC;
    b.fpart = EMAC0.TIMNANO;
    // correct for access skew at second boundary
    if(b.fpart < a.fpart && b.ipart == a.ipart) ++b.ipart;

    b.fpart <<= 1;
    return b.full;
}
