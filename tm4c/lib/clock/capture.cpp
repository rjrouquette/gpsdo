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

#include <cmath>

// sample ring size
static constexpr int RING_SIZE = 16;
// sample ring mask
static constexpr int RING_MASK = RING_SIZE - 1;
// ring head position
static volatile int ringHead = 0;
// ring tail position
static volatile int ringTail = 0;
// ring buffer
static volatile uint32_t ringBuffer[16];

// time-constant scale factor (2 seconds)
static constexpr float TIME_CONSTANT = 0.5f;
// scale factor for converting timer ticks to seconds
static constexpr float timeScale = 1.0f / static_cast<float>(CLK_FREQ);
// ema accumulator for period mean (initialize to zero Celsius)
static volatile float emaPeriodMean = 0;
// ema accumulator for period variance
static volatile float emaPeriodVar = 0;

// capture rising edge of temperature sensor output
void ISR_Timer4B() {
    // clear capture interrupt flag
    GPTM4.ICR = GPTM_ICR_CBE;
    // snapshot edge time
    uint32_t timer = clock::monotonic::raw();
    // determine edge time
    timer -= (timer - GPTM4.TBR.raw) & 0xFFFF;
    // add sample to buffer
    const int next = (ringHead + 1) & RING_MASK;
    ringBuffer[next] = timer;
    ringHead = next;
}

// update mean and standard deviation
static void runTemperature([[maybe_unused]] void *ref) {
    const auto head = ringHead;
    auto tail = ringTail;
    float mean = emaPeriodMean;
    float var = emaPeriodVar;

    // check initial sample
    if (emaPeriodMean == 0) {
        tail = (tail + 1) & RING_MASK;
        // compute cycle period
        const auto next = (tail + 1) & RING_MASK;
        const auto periodRaw = ringBuffer[next] - ringBuffer[tail];
        mean = static_cast<float>(periodRaw) * timeScale;
        tail = next;
    }

    // append new samples
    while(tail != head) {
        // compute cycle period
        const auto next = (tail + 1) & RING_MASK;
        const auto periodRaw = ringBuffer[next] - ringBuffer[tail];
        const auto period = static_cast<float>(periodRaw) * timeScale;
        tail = next;
        // update mean
        const auto diff = period - mean;
        const auto alpha = period * TIME_CONSTANT;
        mean += diff * alpha;
        // update variance
        var += (diff * diff - var) * alpha;
    }

    // apply updates
    ringTail = tail;
    emaPeriodMean = mean;
    emaPeriodVar = var;
}

float clock::capture::temperature() {
    return 0.25f / emaPeriodMean - 273.15f;
}

float clock::capture::temperatureNoise() {
    const auto scale = 1.0f / emaPeriodMean;
    return 0.25f * scale * scale * std::sqrt(emaPeriodVar);
}


// timer tick offset between the ethernet clock and monotonic clock
static volatile uint32_t ppsEthernetOffset = 0;

// capture rising edge of ethernet PPS for offset measurement
void ISR_Timer5A() {
    // clear capture interrupt flag
    GPTM5.ICR = GPTM_ICR_CAE;
    // snapshot edge time
    uint32_t timer = clock::monotonic::raw();
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

// capture rising edge of GPS PPS for offset measurement
void ISR_Timer5B() {
    // clear capture interrupt flag
    GPTM5.ICR = GPTM_ICR_CBE;
    // snapshot edge time
    uint32_t timer = clock::monotonic::raw();
    // update pps edge state
    timer -= (timer - GPTM5.TBR.raw) & 0xFFFF;
    // update edge time
    ppsGpsEvent = timer;
    // trigger computation of full timestamps
    runWake(taskPpsUpdate);
}

static void runPpsGps([[maybe_unused]] void *ref) {
    clock::capture::rawToFull(ppsGpsEvent, const_cast<uint64_t*>(ppsStamp));
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

namespace clock::capture {
    void init();
}

void clock::capture::init() {
    // create capture interrupt worker task
    taskPpsUpdate = runWait(runPpsGps, nullptr);

    // enable capture timers
    RCGCTIMER.raw |= 0x31;
    delay::cycles(4);
    initCaptureTimer(GPTM4);
    initCaptureTimer(GPTM5);
    // synchronize capture timers with monotonic clock
    GPTM0.SYNC = 0x0F03;

    // configure capture pins
    RCGCGPIO.EN_PORTM = 1;
    delay::cycles(4);
    // unlock GPIO config
    PORTM.LOCK = GPIO_LOCK_KEY;
    PORTM.CR = 0xF0u;
    // configure pins;
    PORTM.PCTL.PMC4 = 3;
    PORTM.PCTL.PMC5 = 3;
    PORTM.PCTL.PMC6 = 3;
    PORTM.PCTL.PMC7 = 3;
    PORTM.AFSEL.ALT4 = 1;
    PORTM.AFSEL.ALT5 = 1;
    PORTM.AFSEL.ALT6 = 1;
    PORTM.AFSEL.ALT7 = 1;
    PORTM.DEN = 0xF0u;
    // lock GPIO config
    PORTM.CR = 0;
    PORTM.LOCK = 0;

    // start temperature worker task
    ringHead = 0;
    ringTail = 0;
    runSleep(RUN_SEC / 128, runTemperature, nullptr);
}
