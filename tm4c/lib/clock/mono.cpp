//
// Created by robert on 4/26/23.
//

#include "mono.hpp"

#include "clock.hpp"
#include "util.hpp"
#include "../delay.hpp"
#include "../hw/interrupts.h"
#include "../hw/timer.h"


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

// raw internal monotonic clock state
static volatile uint32_t clkMonoInt = 0;
// raw internal monotonic clock state
static volatile uint32_t clkMonoOff = 0;

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

uint32_t clock::monotonic::seconds() {
    return clkMonoInt;
}

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
    scratch.fpart = nanosToFrac(ticks * CLK_NANOS);
    scratch.ipart = integer;
    return scratch.full;
}
