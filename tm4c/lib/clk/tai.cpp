//
// Created by robert on 4/27/23.
//

#include "tai.hpp"

#include "comp.hpp"
#include "mono.hpp"
#include "util.hpp"
#include "../delay.hpp"
#include "../gps.hpp"
#include "../run.hpp"
#include "../../hw/gpio.h"
#include "../../hw/interrupts.h"
#include "../../hw/timer.h"

#define PPS_TIMER GPTM2
#define PPS_PORT PORTA
#define PPS_PIN (1<<4)
#define PPS_INTV_HI (CLK_FREQ / 10) // 100 ms

volatile uint64_t clkTaiUtcOffset = 0;

volatile uint64_t clkTaiOffset = 0;
volatile uint64_t clkTaiRef = 0;
volatile int32_t clkTaiRate = 0;
static volatile uint32_t clkTaiRem = 0;

static volatile uint32_t clkPpsLow = CLK_FREQ - PPS_INTV_HI - 1;


// PPS output timer
void ISR_Timer2A() {
    // clear timeout interrupt flag
    PPS_TIMER.ICR = GPTM_ICR_TATO;
    // schedule next output transition
    const int isHi = (PPS_TIMER.TAMR.TCACT == 0x3);
    PPS_TIMER.TAMR.TCACT = isHi ? 0x2 : 0x3;
    PPS_TIMER.TAILR = isHi ? (PPS_INTV_HI - 1) : clkPpsLow;
}

static void runClkTai(void *ref) {
    // prepare update values
    const uint64_t now = CLK_COMP();
    const int32_t offset = corrFracRem(clkTaiRate, now - clkTaiRef, clkTaiRem);

    // apply update
    __disable_irq();
    clkTaiRef = now;
    clkTaiOffset += offset;
    __enable_irq();
}

static void runPpsTai(void *ref) {
    // update PPS output alignment
    fixed_32_32 scratch = {};
    // use imprecise TAI calculation to reduce overhead
    scratch.full = CLK_MONO();
    scratch.full += clkCompOffset;
    scratch.full += clkTaiOffset;
    // wait for end-of-second
    if (scratch.fpart >= 0xE0000000u) {
        // compute next TAI second boundary
        scratch.fpart = 0;
        ++scratch.ipart;
        uint32_t rem = 0;
        // translate to compensated domain
        scratch.full -= clkTaiOffset;
        scratch.full += corrFracRem(-clkTaiRate, scratch.full - clkTaiRef, rem);
        // translate to monotonic domain
        scratch.full -= clkCompOffset;
        scratch.full += corrFracRem(-clkCompRate, scratch.full - clkCompRef, rem);
        // translate to raw timer ticks
        scratch.full *= CLK_FREQ;
        // compute PPS output interval
        auto delta = static_cast<int32_t>(scratch.ipart - clkMonoPps);
        while (delta <= CLK_FREQ / 2)
            delta += CLK_FREQ;
        while (delta >= CLK_FREQ * 2)
            delta -= CLK_FREQ;
        clkPpsLow = delta - PPS_INTV_HI - 1;
        // update PPS output interval
        __disable_irq();
        if (PPS_TIMER.TAMR.TCACT == 0x3)
            PPS_TIMER.TAILR = clkPpsLow;
        __enable_irq();
    }
}

void initClkTai() {
    // Enable Timer 2
    RCGCTIMER.EN_GPTM2 = 1;
    delay::cycles(4);
    // Configure Timer
    PPS_TIMER.TAILR = CLK_FREQ;
    PPS_TIMER.TAMR.MR = 0x2;
    PPS_TIMER.TAMR.CDIR = 1;
    // set CCP pin on timeout
    PPS_TIMER.TAMR.TCACT = 0x3;
    // enable interrupt
    PPS_TIMER.IMR.TATO = 1;
    // start timer
    PPS_TIMER.CTL.TAEN = 1;
    // reduce interrupt priority
    ISR_priority(ISR_Timer2A, 7);

    // enable CCP output on PA2
    RCGCGPIO.EN_PORTA = 1;
    delay::cycles(4);
    // unlock GPIO config
    PPS_PORT.LOCK = GPIO_LOCK_KEY;
    PPS_PORT.CR = PPS_PIN;
    // configure pins
    PPS_PORT.DIR |= PPS_PIN;
    PPS_PORT.DR8R |= PPS_PIN;
    PPS_PORT.PCTL.PMC4 = 3;
    PPS_PORT.AFSEL.ALT4 = 1;
    PPS_PORT.DEN |= PPS_PIN;
    // lock GPIO config
    PPS_PORT.CR = 0;
    PPS_PORT.LOCK = 0;

    // initialize UTC offset
    clkTaiUtcOffset = static_cast<uint64_t>(GPS_taiOffset()) << 32;
    // schedule updates
    runSleep(RUN_SEC / 4, runClkTai, nullptr);
    runSleep(RUN_SEC / 64, runPpsTai, nullptr);
}

uint64_t CLK_TAI() {
    uint32_t rem = 0;
    // get monotonic time
    uint64_t ts = CLK_MONO();
    // translate to compensated domain
    ts += corrFracRem(clkCompRate, ts - clkCompRef, rem);
    ts += clkCompOffset;
    // translate to TAI domain
    ts += corrFracRem(clkTaiRate, ts - clkTaiRef, rem);
    ts += clkTaiOffset;
    return ts;
}

uint64_t CLK_TAI_fromMono(uint64_t ts) {
    // translate to compensated domain
    ts += corrValue(clkCompRate, static_cast<int64_t>(ts - clkCompRef));
    ts += clkCompOffset;
    // translate to TAI domain
    ts += corrValue(clkTaiRate, static_cast<int64_t>(ts - clkTaiRef));
    ts += clkTaiOffset;
    return ts;
}

void CLK_TAI_setTrim(int32_t comp) {
    // prepare update values
    const uint64_t now = CLK_COMP();
    const int32_t offset = corrFracRem(clkTaiRate, now - clkTaiRef, clkTaiRem);

    // apply update
    __disable_irq();
    clkTaiRef = now;
    clkTaiRate = comp;
    clkTaiOffset += offset;
    __enable_irq();
}

int32_t CLK_TAI_getTrim() {
    return clkTaiRate;
}

void CLK_TAI_adjust(int64_t delta) {
    __disable_irq();
    clkTaiOffset += delta;
    __enable_irq();
}
