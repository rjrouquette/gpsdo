//
// Created by robert on 4/26/23.
//

#include "../../hw/gpio.h"
#include "../../hw/interrupts.h"
#include "../../hw/timer.h"
#include "../delay.h"
#include "mono.h"
#include "comp.h"
#include "util.h"

#define MIN_UPDT_INTV (CLK_FREQ / 4) // 4 Hz

#define FREQ_TIMER GPTM1
#define FREQ_PORT PORTA
#define FREQ_PIN (1<<2)
#define FREQ_INTV (CLK_FREQ / 2000) // 1 kHz

static volatile uint32_t clkCompUpdated = 0;
static volatile uint32_t clkCompRem = 0;
volatile uint64_t clkCompOffset = 0;
volatile uint64_t clkCompRef = 0;
volatile int32_t clkCompRate = 0;

static volatile uint64_t freqOutInc = 0;
static volatile uint32_t freqOutRem = 0;


// frequency output timer
void ISR_Timer1A() {
    // update interval adjustment
    union fixed_32_32 scratch;
    scratch.full = freqOutInc;
    scratch.full += freqOutRem;
    freqOutRem = scratch.fpart;
    // set next second boundary
    FREQ_TIMER.TAILR = scratch.ipart;
    // clear interrupt flag
    FREQ_TIMER.ICR.TATO = 1;
}

void initClkComp() {
    // Enable Timer 1
    RCGCTIMER.EN_GPTM1 = 1;
    delay_cycles_4();
    // Configure Timer
    FREQ_TIMER.TAILR = FREQ_INTV - 1;
    FREQ_TIMER.TAMR.MR = 0x2;
    FREQ_TIMER.TAMR.CDIR = 1;
    // toggle CCP pin on timeout
    FREQ_TIMER.TAMR.TCACT = 0x1;
    // enable interrupt
    FREQ_TIMER.IMR.TATO = 1;
    // start timer
    FREQ_TIMER.CTL.TAEN = 1;

    // enable CCP output on PA2
    RCGCGPIO.EN_PORTA = 1;
    delay_cycles_4();
    // unlock GPIO config
    FREQ_PORT.LOCK = GPIO_LOCK_KEY;
    FREQ_PORT.CR = FREQ_PIN;
    // configure pins
    FREQ_PORT.DIR |= FREQ_PIN;
    FREQ_PORT.DR8R |= FREQ_PIN;
    FREQ_PORT.PCTL.PMC2 = 3;
    FREQ_PORT.AFSEL.ALT2 = 1;
    FREQ_PORT.DEN |= FREQ_PIN;
    // lock GPIO config
    FREQ_PORT.CR = 0;
    FREQ_PORT.LOCK = 0;

    CLK_COMP_setComp(0);
}

void runClkComp() {
    // periodically update alignment to prevent numerical overflow
    if((GPTM0.TAV.raw - clkCompUpdated) >= MIN_UPDT_INTV) {
        clkCompUpdated += MIN_UPDT_INTV;
        CLK_COMP_setComp(clkCompRate);
    }
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
    uint64_t now = CLK_MONO();
    uint64_t offset = corrFrac(clkCompRate, (uint32_t) (now - clkCompRef), &clkCompRem);

    // apply update
    __disable_irq();
    clkCompRef = now;
    clkCompRate = comp;
    clkCompOffset += offset;
    __enable_irq();

    // update frequency output
    union fixed_32_32 incr;
    incr.ipart = 0;
    incr.fpart = FREQ_INTV;
    incr.full *= -comp;
    incr.ipart += FREQ_INTV - 1;
    __disable_irq();
    freqOutInc = incr.full;
    __disable_irq();
}

int32_t CLK_COMP_getComp() {
    return clkCompRate;
}
