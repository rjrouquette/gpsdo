//
// Created by robert on 4/26/23.
//

#include "../delay.h"
#include "../hw/emac.h"
#include "../hw/gpio.h"
#include "../hw/interrupts.h"
#include "../hw/sys.h"
#include "../hw/timer.h"
#include "mono.h"
#include "tai.h"
#include "trim.h"
#include "util.h"


volatile uint32_t clkMonoInt = 0;
volatile uint32_t clkMonoOff = 0;
volatile int64_t clkMonoEth = 0;

extern uint64_t taiOffset;

// pps edge capture state
volatile struct ClockEvent clkMonoPps;


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

static void initClkMono() {
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

static void initClkRxTx() {
    // disable flash prefetch per errata
    FLASHCONF.FPFOFF = 1;
    // enable clock
    RCGCEMAC.EN0 = 1;
    delay_cycles_4();

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

void initClkSync() {
    // Enable Timer 5
    RCGCTIMER.EN_GPTM5 = 1;
    delay_cycles_4();
    // Configure Timer 5 for capture mode
    GPTM5.CFG.GPTMCFG = 4;
    GPTM5.TAMR.MR = 0x3;
    GPTM5.TBMR.MR = 0x3;
    // edge-time mode
    GPTM5.TAMR.CMR = 1;
    GPTM5.TBMR.CMR = 1;
    // rising edge
    GPTM5.CTL.TAEVENT = 0;
    GPTM5.CTL.TBEVENT = 0;
    // count up
    GPTM5.TAMR.CDIR = 1;
    GPTM5.TBMR.CDIR = 1;
    // disable overflow interrupt
    GPTM5.TAMR.CINTD = 0;
    GPTM5.TBMR.CINTD = 0;
    // full count range
    GPTM5.TAILR = -1;
    GPTM5.TBILR = -1;
    // interrupts
    GPTM5.IMR.CAE = 1;
    GPTM5.IMR.CBE = 1;
    // start timer
    GPTM5.CTL.TAEN = 1;
    GPTM5.CTL.TBEN = 1;
    // synchronize with monotonic clock
    GPTM0.SYNC = 0x0c03;

    // configure capture pins
    RCGCGPIO.EN_PORTM = 1;
    delay_cycles_4();
    // unlock GPIO config
    PORTM.LOCK = GPIO_LOCK_KEY;
    PORTM.CR = 0xC0u;
    // configure pins;
    PORTM.PCTL.PMC6 = 3;
    PORTM.PCTL.PMC7 = 3;
    PORTM.AFSEL.ALT6 = 1;
    PORTM.AFSEL.ALT7 = 1;
    PORTM.DEN = 0xC0u;
    // lock GPIO config
    PORTM.CR = 0;
    PORTM.LOCK = 0;

    // configure PPS output pin
    RCGCGPIO.EN_PORTG = 1;
    delay_cycles_4();
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
}

void CLK_MONO_init() {
    initClkSys();
    initClkMono();
    initClkRxTx();
    initClkSync();
}

// second boundary comparison
void ISR_Timer0A() {
    // increment counter
    clkMonoOff += CLK_FREQ;
    ++clkMonoInt;
    // set next second boundary
    GPTM0.TAMATCHR = clkMonoOff + CLK_FREQ;
    // clear interrupt flag
    GPTM0.ICR.TAM = 1;
}

// return integer part of current time
uint32_t CLK_MONO_INT() {
    return clkMonoInt;
}
// public alias
uint32_t CLK_MONOTONIC_INT() __attribute__((alias("CLK_MONO_INT")));

// return current time as 32.32 fixed point value
uint64_t CLK_MONO() {
    // capture current time
    __disable_irq();
    uint32_t snapF = GPTM0.TAV.raw;
    uint32_t snapI = clkMonoInt;
    uint32_t snapO = clkMonoOff;
    __enable_irq();
    // convert to timestamp
    return fromClkMono(snapF, snapO, snapI);
}
// public alias
uint64_t CLK_MONOTONIC() __attribute__((alias("CLK_MONO")));

// return timestamp of PPS edge capture as 32.32 fixed point value
uint64_t CLK_MONO_PPS() {
    return fromClkMono(clkMonoPps.timer, clkMonoPps.offset, clkMonoPps.integer);
}


// capture rising edge of ethernet PPS for offset measurement
void ISR_Timer5A() {
    // snapshot edge time
    uint32_t timer = GPTM0.TAV.raw;
    uint32_t event = GPTM5.TAR.raw;
    // clear interrupt flag
    GPTM5.ICR.CAE = 1;
    // compute ethernet clock offset
    timer -= (timer - event) & 0xFFFF;
    timer -= (uint32_t) (EMAC0.TIMSEC * CLK_FREQ);
    clkMonoEth = timer;
}

// capture rising edge of GPS PPS for offset measurement
void ISR_Timer5B() {
    // snapshot edge time
    uint32_t timer = GPTM0.TAV.raw;
    uint32_t event = GPTM5.TBR.raw;
    // clear interrupt flag
    GPTM5.ICR.CBE = 1;
    // update pps edge state
    timer -= (timer - event) & 0xFFFF;
    // monotonic clock state
    clkMonoPps.timer = timer;
    clkMonoPps.offset = clkMonoOff;
    clkMonoPps.integer = clkMonoInt;
    // trimmed clock state
    clkMonoPps.trimRate = clkTrimRate;
    clkMonoPps.trimRef = clkTrimRef;
    clkMonoPps.trimOff = clkTrimOffset;
    // tai clock state
    clkMonoPps.taiRate = clkTaiRate;
    clkMonoPps.taiRef = clkTaiRef;
    clkMonoPps.taiOff = clkTaiOffset;
}