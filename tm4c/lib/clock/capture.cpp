//
// Created by robert on 4/17/24.
//

#include "capture.hpp"

#include "comp.hpp"
#include "mono.hpp"
#include "tai.hpp"
#include "util.hpp"
#include "../hw/emac.h"
#include "../hw/interrupts.h"

// capture rising edge of ethernet PPS for offset measurement
void ISR_Timer5A() {
    // snapshot edge time
    uint32_t timer = clock::monotonic::raw();
    const uint32_t event = GPTM5.TAR.raw;
    // clear capture interrupt flag
    GPTM5.ICR = GPTM_ICR_CAE;
    // compute ethernet clock offset
    timer -= (timer - event) & 0xFFFF;
    timer -= EMAC0.TIMSEC * CLK_FREQ;
    clkMonoEth = timer;
}

// capture rising edge of GPS PPS for offset measurement
void ISR_Timer5B() {
    // snapshot edge time
    uint32_t timer = clock::monotonic::raw();
    const uint32_t event = GPTM5.TBR.raw;
    // clear capture interrupt flag
    GPTM5.ICR = GPTM_ICR_CBE;
    // update pps edge state
    timer -= (timer - event) & 0xFFFF;
    // monotonic clock state
    clkMonoPpsEvent.timer = timer;
    clkMonoPpsEvent.offset = clkMonoOff;
    clkMonoPpsEvent.integer = clkMonoInt;
    // compensated clock state
    clkMonoPpsEvent.compRate = clkCompRate;
    clkMonoPpsEvent.compRef = clkCompRef;
    clkMonoPpsEvent.compOff = clkCompOffset;
    // tai clock state
    clkMonoPpsEvent.taiRate = clkTaiRate;
    clkMonoPpsEvent.taiRef = clkTaiRef;
    clkMonoPpsEvent.taiOff = clkTaiOffset;
}

// internal state for clock::capture::pps (reduces overhead)
static volatile ClockEvent ppsEvent;
static volatile uint64_t ppsStamp[3];

void clock::capture::ppsGps(uint64_t *tsResult) {
    if (
        ppsEvent.timer == clkMonoPpsEvent.timer &&
        ppsEvent.integer == clkMonoPpsEvent.integer
    ) {
        tsResult[0] = ppsStamp[0];
        tsResult[1] = ppsStamp[1];
        tsResult[2] = ppsStamp[2];
        return;
    }

    __disable_irq();
    const_cast<ClockEvent&>(ppsEvent) = const_cast<ClockEvent&>(clkMonoPpsEvent);
    __enable_irq();

    // convert snapshot to timestamps
    uint32_t rem = 0;
    // monotonic clock
    ppsStamp[0] = fromClkMono(ppsEvent.timer, ppsEvent.offset, ppsEvent.integer);
    // frequency compensated clock
    ppsStamp[1] = ppsStamp[0] +
                  corrFracRem(ppsEvent.compRate, ppsStamp[0] - ppsEvent.compRef, rem) +
                  ppsEvent.compOff;
    // TAI disciplined clock
    ppsStamp[2] = ppsStamp[1] +
                  corrFracRem(ppsEvent.taiRate, ppsStamp[1] - ppsEvent.taiRef, rem) +
                  ppsEvent.taiOff;

    // return result
    tsResult[0] = ppsStamp[0];
    tsResult[1] = ppsStamp[1];
    tsResult[2] = ppsStamp[2];
}

void clock::capture::toStamps(const uint32_t monoRaw, uint64_t *stamps) {
    // snapshot clock state
    __disable_irq();
    const uint32_t offset = clkMonoOff;
    const uint32_t integer = clkMonoInt;
    __enable_irq();

    // monotonic clock
    stamps[0] = fromClkMono(monoRaw, offset, integer);

    // frequency compensated clock
    uint32_t rem = 0;
    stamps[1] = stamps[0] +
                corrFracRem(clkCompRate, stamps[0] - clkCompRef, rem) +
                clkCompOffset;

    // TAI disciplined clock
    stamps[2] = stamps[1] +
                corrFracRem(clkTaiRate, stamps[1] - clkTaiRef, rem) +
                clkTaiOffset;
}
