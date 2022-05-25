//
// Created by robert on 4/15/22.
//

#include "../hw/adc.h"
#include "../hw/sys.h"
#include "../hw/timer.h"
#include "clk.h"
#include "delay.h"

#define USE_XTAL 1
#define CLK_FREQ (125000000)
#define CLK_MOD (32u*125000000u)

static volatile uint32_t cntMonotonic = 0;
static volatile uint32_t taiFracOffset = 0;
static volatile uint32_t taiIntOffset = -1;

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
    // trigger temperature measurement
    ADC0.PSSI.SS3 = 1;
}

// return integer part of current time
uint32_t CLK_MONOTONIC_INT() {
    return cntMonotonic;
}

// return current time as 32.32 fixed point value
uint64_t CLK_MONOTONIC() {
    // result structure
    register union {
        struct {
            uint32_t fpart;
            uint32_t ipart;
        };
        uint64_t full;
    } result;

    // capture current time
    register uint32_t snapI = cntMonotonic;
    result.fpart = GPTM0.TAV.raw;

    // split fractional time into fraction and remainder
    result.ipart = result.fpart;
    result.ipart /= 1953125;
    result.fpart -= 1953125 * result.ipart;

    // compute twiddle for fractional conversion
    register uint32_t twiddle = result.fpart;
    twiddle *= 1524;
    twiddle >>= 16;
    // apply fractional coefficient
    result.fpart *= 2199;
    // apply fraction twiddle
    result.fpart += twiddle;
    // adjust bit alignment
    result.full >>= 6;

    // merge with full integer count
    result.ipart ^= snapI;
    result.ipart &= 1;
    result.ipart += snapI;
    return result.full;
}

uint64_t CLK_TAI() {
    uint64_t result = CLK_MONOTONIC();
    result += taiFracOffset;
    ((uint32_t*)&result)[1] += taiIntOffset;
    return result;
}

void CLK_setFracTAI(uint32_t offset) {
    taiFracOffset = offset;
}
