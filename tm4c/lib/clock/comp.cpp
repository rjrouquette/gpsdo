//
// Created by robert on 4/26/23.
//

#include "comp.hpp"

#include "mono.hpp"
#include "util.hpp"
#include "../delay.hpp"
#include "../run.hpp"
#include "../../hw/gpio.h"
#include "../../hw/interrupts.h"
#include "../../hw/timer.h"


#define FRQ_TIMER GPTM1
#define FRQ_PORT PORTA
#define FRQ_PIN (1<<2)
static constexpr int INTERVAL = CLK_FREQ / 2000; // 1 kHz

volatile uint64_t clkCompOffset = 0;
volatile uint64_t clkCompRef = 0;
volatile int32_t clkCompRate = 0;
static volatile uint32_t clkCompRem = 0;

static uint64_t frqInc;
static uint32_t frqRem;


// frequency output timer
void ISR_Timer1A() {
    // update interval adjustment
    const uint64_t scratch = frqInc + frqRem;
    frqRem = scratch;
    // set next second boundary
    FRQ_TIMER.TAILR = scratch >> 32;
    // clear timeout interrupt flag
    FRQ_TIMER.ICR = GPTM_ICR_TATO;
}

void runClkComp(void *ref) {
    // prepare update values
    const uint64_t now = clock::monotonic::now();
    const int32_t offset = corrFracRem(clkCompRate, now - clkCompRef, clkCompRem);

    // apply update
    clkCompRef = now;
    clkCompOffset += offset;
}

void initClkComp() {
    // Enable Timer 1
    RCGCTIMER.EN_GPTM1 = 1;
    delay::cycles(4);
    // Configure Timer
    FRQ_TIMER.TAILR = INTERVAL - 1;
    FRQ_TIMER.TAMR.MR = 0x2;
    FRQ_TIMER.TAMR.CDIR = 1;
    // toggle CCP pin on timeout
    FRQ_TIMER.TAMR.TCACT = 0x1;
    // enable interrupt
    FRQ_TIMER.IMR.TATO = 1;
    // start timer
    FRQ_TIMER.CTL.TAEN = 1;
    // reduce interrupt priority
    ISR_priority(ISR_Timer1A, 6);

    // enable CCP output on PA2
    RCGCGPIO.EN_PORTA = 1;
    delay::cycles(4);
    // unlock GPIO config
    FRQ_PORT.LOCK = GPIO_LOCK_KEY;
    FRQ_PORT.CR = FRQ_PIN;
    // configure pins
    FRQ_PORT.DIR |= FRQ_PIN;
    FRQ_PORT.DR8R |= FRQ_PIN;
    FRQ_PORT.PCTL.PMC2 = 3;
    FRQ_PORT.AFSEL.ALT2 = 1;
    FRQ_PORT.DEN |= FRQ_PIN;
    // lock GPIO config
    FRQ_PORT.CR = 0;
    FRQ_PORT.LOCK = 0;

    clock::compensated::setTrim(0);
    // schedule updates
    runSleep(RUN_SEC / 4, runClkComp, nullptr);
}

uint64_t clock::compensated::now() {
    // get monotonic time
    const uint64_t clkMono = monotonic::now();
    // translate to compensated domain
    int64_t scratch = static_cast<int32_t>(clkMono - clkCompRef);
    scratch *= clkCompRate;
    return clkMono + clkCompOffset + static_cast<int32_t>(scratch >> 32);
}

uint64_t clock::compensated::fromMono(uint64_t ts) {
    ts += corrValue(clkCompRate, static_cast<int64_t>(ts - clkCompRef));
    ts += clkCompOffset;
    return ts;
}

void clock::compensated::setTrim(const int32_t rate) {
    // prepare compensation update
    const uint64_t now = monotonic::now();
    const uint64_t offset = clkCompOffset + corrFracRem(clkCompRate, now - clkCompRef, clkCompRem);

    // prepare frequency output update
    const uint64_t incr = (static_cast<int64_t>(rate) * -INTERVAL) +
                          (static_cast<uint64_t>(INTERVAL - 1) << 32);

    // apply update
    clkCompRate = rate;
    clkCompRef = now;
    clkCompOffset = offset;
    frqInc = incr;
}

int32_t clock::compensated::getTrim() {
    return clkCompRate;
}
