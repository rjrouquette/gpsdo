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

static volatile uint32_t clkCompRem = 0;
volatile uint64_t clkCompOffset = 0;
volatile uint64_t clkCompRef = 0;
volatile int32_t clkCompRate = 0;

static volatile uint64_t frqInc;
static volatile uint32_t frqRem;


// frequency output timer
void ISR_Timer1A() {
    // update interval adjustment
    uint64_t scratch = frqInc + frqRem;
    frqRem = scratch;
    // set next second boundary
    FRQ_TIMER.TAILR = scratch >> 32;
    // clear interrupt flag
    FRQ_TIMER.ICR.TATO = 1;
}

void runClkComp(void *ref) {
    // prepare update values
    const uint64_t now = CLK_MONO();
    const int32_t offset = corrFrac(clkCompRate, now - clkCompRef, &clkCompRem);

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
    runInterval(1u << (32 - 2), runClkComp, NULL);
}

uint64_t CLK_COMP() {
    // get monotonic time
    uint64_t scratch = CLK_MONO();
    uint32_t rem = 0;
    // translate to compensated domain
    scratch += corrFrac(clkCompRate, scratch - clkCompRef, &rem);
    scratch += clkCompOffset;
    return scratch;
}

uint64_t CLK_COMP_fromMono(uint64_t ts) {
    ts += corrValue(clkCompRate, (int64_t) (ts - clkCompRef));
    ts += clkCompOffset;
    return ts;
}

void CLK_COMP_setComp(int32_t comp) {
    // prepare update values
    const uint64_t now = CLK_MONO();
    const int32_t offset = corrFrac(clkCompRate, now - clkCompRef, &clkCompRem);

    // apply update
    __disable_irq();
    clkCompRef = now;
    clkCompRate = comp;
    clkCompOffset += offset;
    __enable_irq();

    // prepare update value
    union fixed_32_32 incr;
    incr.ipart = 0;
    incr.fpart = FRQ_INTV;
    incr.full *= -comp;
    incr.ipart += FRQ_INTV - 1;

    // apply update
    __disable_irq();
    frqInc = incr.full;
    __enable_irq();
}

int32_t CLK_COMP_getComp() {
    return clkCompRate;
}
