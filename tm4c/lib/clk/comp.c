//
// Created by robert on 4/26/23.
//

#include <stddef.h>
#include "../../hw/gpio.h"
#include "../../hw/interrupts.h"
#include "../../hw/timer.h"
#include "../delay.h"
#include "../run.h"
#include "mono.h"
#include "comp.h"
#include "util.h"

#define FRQ_TIMER GPTM1
#define FRQ_PORT PORTA
#define FRQ_PIN (1<<2)
#define FRQ_INTV (CLK_FREQ / 2000) // 1 kHz

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
    FRQ_TIMER.ICR.raw = 1u << 0;
}

void runClkComp(void *ref) {
    // prepare update values
    const uint64_t now = CLK_MONO();
    const int32_t offset = corrFracRem64(clkCompRate, now - clkCompRef, &clkCompRem);

    // apply update
    __disable_irq();
    clkCompRef = now;
    clkCompOffset += offset;
    __enable_irq();
}

void initClkComp() {
    // Enable Timer 1
    RCGCTIMER.EN_GPTM1 = 1;
    delay_cycles_4();
    // Configure Timer
    FRQ_TIMER.TAILR = FRQ_INTV - 1;
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
    delay_cycles_4();
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

    CLK_COMP_setComp(0);
    // schedule updates
    runSleep(RUN_SEC >> 2, runClkComp, NULL);
}

uint64_t CLK_COMP() {
    // get monotonic time
    const uint64_t clkMono = CLK_MONO();
    // translate to compensated domain
    int64_t scratch = (int32_t) (clkMono - clkCompRef);
    scratch *= clkCompRate;
    return clkMono + clkCompOffset + (int32_t) (scratch >> 32);
}

uint64_t CLK_COMP_fromMono(uint64_t ts) {
    ts -= clkCompRef;
    ts += corrValue(clkCompRate, (int64_t) ts);
    ts += clkCompOffset;
    return ts;
}

void CLK_COMP_setComp(int32_t comp) {
    // prepare compensation update
    const uint64_t now = CLK_MONO();
    const uint64_t offset = clkCompOffset + corrFracRem64(clkCompRate, now - clkCompRef, &clkCompRem);

    // prepare frequency output update
    const uint64_t incr = (((int64_t) comp) * -FRQ_INTV) + (((uint64_t) (FRQ_INTV - 1)) << 32);

    // apply update
    __disable_irq();
    clkCompRate = comp;
    clkCompRef = now;
    clkCompOffset = offset;
    frqInc = incr;
    __enable_irq();
}

int32_t CLK_COMP_getComp() {
    return clkCompRate;
}
