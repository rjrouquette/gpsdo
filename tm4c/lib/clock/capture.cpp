//
// Created by robert on 4/17/24.
//

#include "capture.hpp"

#include "comp.hpp"
#include "mono.hpp"
#include "tai.hpp"
#include "util.hpp"
#include "../run.hpp"
#include "../hw/emac.h"
#include "../hw/interrupts.h"


// timer tick offset between the ethernet clock and monotonic clock
static volatile uint32_t ppsEthernetOffset = 0;

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
    ppsEthernetOffset = timer;
}

uint32_t clock::capture::ppsEthernetRaw() {
    return ppsEthernetOffset;
}

// timer time of most recent GPS PPS
static volatile uint32_t ppsGpsEvent;
// full 64-bit fixed point timestamps
static volatile uint64_t ppsStamp[3];
// task handle for timestamp update
static void *volatile taskPpsUpdate;

static void runPpsGps([[maybe_unused]] void *ref) {
    clock::capture::rawToFull(ppsGpsEvent, const_cast<uint64_t*>(ppsStamp));
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
    // update edge time
    ppsGpsEvent = timer;
    // trigger computation of full timestamps
    runWake(taskPpsUpdate);
}

void clock::capture::ppsGps(uint64_t *tsResult) {
    tsResult[0] = ppsStamp[0];
    tsResult[1] = ppsStamp[1];
    tsResult[2] = ppsStamp[2];
}

void clock::capture::rawToFull(const uint32_t monoRaw, uint64_t *stamps) {
    // monotonic clock
    stamps[0] = monotonic::fromRaw(monoRaw);

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

namespace clock::capture {
    void init() {
        taskPpsUpdate = runWait(runPpsGps, nullptr);
    }
}
