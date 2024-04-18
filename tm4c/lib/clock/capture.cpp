//
// Created by robert on 4/17/24.
//

#include "capture.hpp"

#include "comp.hpp"
#include "mono.hpp"
#include "tai.hpp"
#include "util.hpp"
#include "../delay.hpp"
#include "../run.hpp"
#include "../hw/emac.h"
#include "../hw/gpio.h"
#include "../hw/interrupts.h"


// timer tick offset between the ethernet clock and monotonic clock
static volatile uint32_t ppsEthernetOffset = 0;

// capture rising edge of ethernet PPS for offset measurement
void ISR_Timer5A() {
    // snapshot edge time
    uint32_t timer = clock::monotonic::raw();
    // clear capture interrupt flag
    GPTM5.ICR = GPTM_ICR_CAE;
    // compute ethernet clock offset
    timer -= (timer - GPTM5.TAR.raw) & 0xFFFF;
    timer -= EMAC0.TIMSEC * CLK_FREQ;
    // update edge offset
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
    // clear capture interrupt flag
    GPTM5.ICR = GPTM_ICR_CBE;
    // update pps edge state
    timer -= (timer - GPTM5.TBR.raw) & 0xFFFF;
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
    void init();
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

void clock::capture::init() {
    // create capture interrupt worker task
    taskPpsUpdate = runWait(runPpsGps, nullptr);

    // enable capture timers
    RCGCTIMER.raw |= 0x31;
    delay::cycles(4);
    initCaptureTimer(GPTM4);
    initCaptureTimer(GPTM5);
    // disable timer 4B interrupt
    GPTM4.IMR.CBE = 0;
    // synchronize capture timers with monotonic clock
    GPTM0.SYNC = 0x0F03;

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
