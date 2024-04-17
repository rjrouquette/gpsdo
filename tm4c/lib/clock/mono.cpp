//
// Created by robert on 4/26/23.
//

#include "mono.hpp"

#include "util.hpp"
#include "../delay.hpp"
#include "../hw/emac.h"
#include "../hw/gpio.h"
#include "../hw/interrupts.h"
#include "../hw/sys.h"
#include "../hw/timer.h"

volatile uint32_t clkMonoInt = 0;
volatile uint32_t clkMonoOff = 0;
volatile uint32_t clkMonoPps = 0;


void initClkMono() {
    // Enable Timer 0
    RCGCTIMER.EN_GPTM0 = 1;
    delay::cycles(4);
    // Configure Timer 0
    TIMER_MONO.TAILR = -1;
    TIMER_MONO.TAMATCHR = CLK_FREQ;
    TIMER_MONO.TAMR.MR = 0x2;
    TIMER_MONO.TAMR.CDIR = 1;
    TIMER_MONO.TAMR.CINTD = 0;
    TIMER_MONO.IMR.TAM = 1;
    TIMER_MONO.TAMR.MIE = 1;
    // start timer
    TIMER_MONO.CTL.TAEN = 1;
}

void initClkEth() {
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

static void initCaptureTimer(volatile GPTM_MAP &timer) {
    // configure timer for capture mode
    timer.CFG.GPTMCFG = 4;
    timer.TAMR.MR = 0x3;
    timer.TBMR.MR = 0x3;
    // edge-time mode
    timer.TAMR.CMR = 1;
    timer.TBMR.CMR = 1;
    // rising edge
    timer.CTL.TAEVENT = 0;
    timer.CTL.TBEVENT = 0;
    // count up
    timer.TAMR.CDIR = 1;
    timer.TBMR.CDIR = 1;
    // disable overflow interrupt
    timer.TAMR.CINTD = 0;
    timer.TBMR.CINTD = 0;
    // full count range
    timer.TAILR = -1;
    timer.TBILR = -1;
    // interrupts
    timer.IMR.CAE = 1;
    timer.IMR.CBE = 1;
    // start timer
    timer.CTL.TAEN = 1;
    timer.CTL.TBEN = 1;
}

void initClkSync() {
    // enable capture timers
    RCGCTIMER.raw |= 0x31;
    delay::cycles(4);
    initCaptureTimer(GPTM4);
    initCaptureTimer(GPTM5);
    // disable timer 4B interrupt
    GPTM4.IMR.CBE = 0;
    // synchronize capture timers with monotonic clock
    TIMER_MONO.SYNC = 0x0F03;

    // configure capture pins
    RCGCGPIO.EN_PORTM = 1;
    delay::cycles(4);
    // unlock GPIO config
    PORTM.LOCK = GPIO_LOCK_KEY;
    PORTM.CR = 0xD0u;
    // configure pins;
    PORTM.PCTL.PMC4 = 3;
    PORTM.PCTL.PMC6 = 3;
    PORTM.PCTL.PMC7 = 3;
    PORTM.AFSEL.ALT4 = 1;
    PORTM.AFSEL.ALT6 = 1;
    PORTM.AFSEL.ALT7 = 1;
    PORTM.DEN = 0xD0u;
    // lock GPIO config
    PORTM.CR = 0;
    PORTM.LOCK = 0;
}

// second boundary comparison
void ISR_Timer0A() {
    // increment counter
    clkMonoOff += CLK_FREQ;
    ++clkMonoInt;
    // set next second boundary
    TIMER_MONO.TAMATCHR = clkMonoOff + CLK_FREQ;
    // clear interrupt flag
    TIMER_MONO.ICR = GPTM_ICR_TAM;
}

// return integer part of current time
uint32_t clock::monotonic::seconds() {
    return clkMonoInt;
}

// return current time as 32.32 fixed point value
uint64_t clock::monotonic::now() {
    return fromRaw(raw());
}

uint64_t clock::monotonic::fromRaw(uint32_t monoRaw) {
    __disable_irq();
    uint32_t integer = clkMonoInt;
    uint32_t offset = clkMonoOff;
    __enable_irq();

    auto ticks = static_cast<int32_t>(monoRaw - offset);
    // adjust for underflow
    while (ticks < 0) {
        ticks += CLK_FREQ;
        --integer;
    }
    // adjust for overflow
    while (ticks >= CLK_FREQ) {
        ticks -= CLK_FREQ;
        ++integer;
    }

    // assemble result
    fixed_32_32 scratch = {};
    scratch.fpart = nanosToFrac(ticks * CLK_NANO);
    scratch.ipart = integer;
    return scratch.full;
}


// capture rising edge of PPS output for offset measurement
void ISR_Timer4A() {
    // snapshot edge time
    uint32_t timer = clock::monotonic::raw();
    const uint32_t event = GPTM4.TAR.raw;
    // clear capture interrupt flag
    GPTM4.ICR = GPTM_ICR_CAE;
    // compute pps output time
    timer -= (timer - event) & 0xFFFF;
    clkMonoPps = timer;
}

// currently unused, but included for completeness
void ISR_Timer4B() {
    // clear capture interrupt flag
    GPTM4.ICR = GPTM_ICR_CBE;
}
