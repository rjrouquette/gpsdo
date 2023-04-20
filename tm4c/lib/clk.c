//
// Created by robert on 4/15/22.
//

#include <math.h>
#include "../hw/emac.h"
#include "../hw/interrupts.h"
#include "../hw/sys.h"
#include "../hw/timer.h"
#include "clk.h"
#include "delay.h"

#define USE_XTAL 1
#define CLK_FREQ (125000000)
#define MAX_TRIM (500e-6f)

static volatile uint32_t monoInt = 0;
static volatile uint32_t monoOff = 0;

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
    GPTM0.TAILR = -1;
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
    // increment counter
    cpsid();
    ++monoInt;
    monoOff = GPTM0.TAMATCHR;
    cpsie();
    // set next second boundary
    GPTM0.TAMATCHR += CLK_FREQ;
    // clear interrupt flag
    GPTM0.ICR.TAM = 1;
}

// return integer part of current time
uint32_t CLK_MONOTONIC_INT() {
    return monoInt;
}

// return current time as 32.32 fixed point value
uint64_t CLK_MONOTONIC() {
    // capture current time
    cpsid();
    register uint32_t snapI = monoInt;
    register uint32_t snapO = monoOff;
    register uint32_t snapF = GPTM0.TAV.raw;
    cpsie();

    // adjust for overflow
    snapF -= snapO;
    while(snapF >= CLK_FREQ) {
        ++snapI;
        snapF -= CLK_FREQ;
    }

    // result structure
    register union {
        struct {
            uint32_t fpart;
            uint32_t ipart;
        };
        uint64_t full;
    } result;

    // assemble result
    result.fpart = nanosToFrac(snapF << 3);
    result.ipart = snapI;
    return result.full;
}

uint32_t CLK_TAI_INT() {
    return EMAC0.TIMSEC;
}

uint64_t CLK_TAI() {
    // result structure
    register union {
        struct {
            uint32_t fpart;
            uint32_t ipart;
        };
        uint64_t full;
    } a, b;

    // load current TAI value
    cpsid();
    a.ipart = EMAC0.TIMSEC;
    a.fpart = EMAC0.TIMNANO;
    b.ipart = EMAC0.TIMSEC;
    b.fpart = EMAC0.TIMNANO;
    cpsie();
    // correct for access skew at second boundary
    if(b.fpart < a.fpart && b.ipart == a.ipart) ++b.ipart;
    // adjust fraction point
    b.fpart <<= 1;
    return b.full;
}

void CLK_TAI_align(int32_t fraction) {
    // wait for hardware ready state
    while(EMAC0.TIMSTCTRL.TSUPDT);
    // set fractional register (convert from two's complement form)
    if(fraction < 0)
        EMAC0.TIMNANOU.raw = (1 << 31) -fraction;
    else
        EMAC0.TIMNANOU.raw = fraction;
    // clear seconds register
    EMAC0.TIMSECU = 0;
    // start update
    EMAC0.TIMSTCTRL.TSUPDT = 1;
    // wait for update to complete
    while(EMAC0.TIMSTCTRL.TSUPDT);
}

void CLK_TAI_set(uint32_t seconds) {
    // wait for hardware ready state
    while(EMAC0.TIMSTCTRL.TSUPDT);
    uint32_t now = EMAC0.TIMSEC;
    // skip if clock is already set
    if(now == seconds) return;
    // set update registers
    if(seconds < now) {
        EMAC0.TIMNANOU.raw = (1 << 31);
        EMAC0.TIMSECU = now - seconds;
    } else {
        EMAC0.TIMNANOU.raw = 0;
        EMAC0.TIMSECU = seconds - now;
    }
    // start update
    EMAC0.TIMSTCTRL.TSUPDT = 1;
    // wait for update to complete
    while(EMAC0.TIMSTCTRL.TSUPDT);
}

float CLK_TAI_trim(float trim) {
    // restrict trim rate
    if(trim < -MAX_TRIM) trim = -MAX_TRIM;
    if(trim >  MAX_TRIM) trim =  MAX_TRIM;
    // convert to correction factor
    int32_t correction = lroundf(trim * 0x1p32f);
    // apply correction update
    EMAC0.TIMADD = 0xFFB34C02 + correction;
    EMAC0.TIMSTCTRL.ADDREGUP = 1;
    // return actual value
    return 0x1p-32f * (float) correction;
}

uint32_t nanosToFrac(uint32_t nanos) {
    // scratch structure
    register union {
        struct {
            uint32_t fpart;
            uint32_t ipart;
        };
        uint64_t full;
    } scratch;

    // multiply by 4 (integer portion of 4.294967296)
    nanos <<= 2;

    // apply correction factor to value
    scratch.ipart = 0;
    scratch.fpart = nanos;
    scratch.full *= 0x12E0BE82;

    // combine correction value with base value
    scratch.ipart += nanos;

    // round up to minimize average error
    scratch.ipart += scratch.fpart >> 31;

    // return result
    return scratch.ipart;
}
