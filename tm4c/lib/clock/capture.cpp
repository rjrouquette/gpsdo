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

// mask for timer edge events
static constexpr int EDGE_MASK = (1 << 24) - 1;

// sample ring size
static constexpr int RING_SIZE = 256;
// sample ring mask
static constexpr int RING_MASK = RING_SIZE - 1;
// ring position
static volatile int ringPos = 0;
// ring buffer
static volatile uint32_t ringBuffer[RING_SIZE];

// scale factor for converting timer ticks to seconds
static constexpr float timeScale = 1.0f / static_cast<float>(CLK_FREQ);
// temperature measurement
static volatile float tempValue = 0;
// temperature initialization counter
static volatile int initCounter = 0;

// capture rising edge of temperature sensor output
void ISR_Timer4B() {
    // clear capture interrupt flag
    GPTM4.ICR = GPTM_ICR_CBE;
    // snapshot edge time
    uint32_t timer = clock::monotonic::raw();
    // determine edge time
    timer -= (timer - GPTM4.TBR.raw) & EDGE_MASK;
    // add sample to buffer
    const int next = (ringPos + 1) & RING_MASK;
    ringBuffer[next] = timer;
    ringPos = next;
}

// update mean and standard deviation
static void runTemperature([[maybe_unused]] void *ref) {
    if(initCounter < 16) {
        ++initCounter;
        return;
    }

    // constant terms
    static constexpr int mid = RING_MASK / 2;
    static constexpr auto cc_ = static_cast<float>(mid + 1);
    static constexpr auto xc_ = 0.5f * cc_ * (cc_ - 1.0f);
    static constexpr auto xx_ = 2.0f * (cc_ - 0.5f) * xc_ / 3.0f;
    static constexpr auto scale = 0.5f * xx_ / timeScale;

    // compute slope
    const auto pivot = ringPos - mid;
    const auto zero = ringBuffer[pivot & RING_MASK];
    float yx = 0;
    for (int i = -mid; i <= mid; ++i) {
        const auto x = static_cast<float>(i);
        const auto y = static_cast<float>(static_cast<int32_t>(zero - ringBuffer[(pivot - i) & RING_MASK]));
        yx += y * x;
    }

    // update temperature measurement
    tempValue = scale / yx - 273.15f;
}

float clock::capture::temperature() {
    return tempValue;
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
    timer -= (timer - GPTM5.TAR.raw) & EDGE_MASK;
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
    timer -= (timer - GPTM5.TBR.raw) & EDGE_MASK;
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
    // full 24-bit extension via pre-scaler
    timer.TAPR = 255;
    timer.TBPR = 255;
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
    runSleep(RUN_SEC / 32, runTemperature, nullptr);
}
