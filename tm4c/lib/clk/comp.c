//
// Created by robert on 4/26/23.
//

#include "../../hw/interrupts.h"
#include "../../hw/timer.h"
#include "mono.h"
#include "comp.h"
#include "util.h"
#include "../delay.h"
#include "../../hw/gpio.h"

#define MIN_UPDT_INTV (CLK_FREQ / 4) // 4 Hz
#define FREQ_OUT_INTV (CLK_FREQ / 2000) // 1 kHz

static volatile uint32_t clkCompUpdated = 0;
static volatile uint32_t clkCompRem = 0;
volatile uint64_t clkCompOffset = 0;
volatile uint64_t clkCompRef = 0;
volatile int32_t clkCompRate = 0;

static volatile uint32_t freqOutRem = 0;


// frequency output edge comparison
void ISR_Timer1A() {
    // update interval adjustment
    union fixed_32_32 scratch;
    scratch.ipart = 0;
    scratch.fpart = FREQ_OUT_INTV;
    scratch.full *= clkCompRate;
    scratch.full += freqOutRem;
    freqOutRem = scratch.fpart;
    // set next second boundary
    GPTM1.TAILR = (FREQ_OUT_INTV - 1) - scratch.ipart;
    // clear interrupt flag
    GPTM1.ICR.TATO = 1;
}

void initClkComp() {
    // Enable Timer 1
    RCGCTIMER.EN_GPTM1 = 1;
    delay_cycles_4();
    // Configure Timer 1
    GPTM1.TAILR = FREQ_OUT_INTV - 1;
    GPTM1.TAMR.MR = 0x2;
    GPTM1.TAMR.CDIR = 1;
    // toggle CCP pin on timeout
    GPTM1.TAMR.TCACT = 0x1;
    // enable interrupt
    GPTM1.TAMR.MIE = 1;
    // start timer
    GPTM1.CTL.TAEN = 1;

    // enable CCP output on PA2
    RCGCGPIO.EN_PORTA = 1;
    delay_cycles_4();
    // unlock GPIO config
    PORTA.LOCK = GPIO_LOCK_KEY;
    PORTA.CR = 0x04u;
    // configure pins
    PORTA.DIR = 0x04u;
    PORTA.DR8R = 0x04u;
    PORTA.PCTL.PMC2 = 3;
    PORTA.AFSEL.ALT2 = 1;
    PORTA.DEN = 0x04u;
    // lock GPIO config
    PORTA.CR = 0;
    PORTA.LOCK = 0;
}

void runClkComp() {
    // periodically update alignment to prevent numerical overflow
    if((GPTM0.TAV.raw - clkCompUpdated) >= MIN_UPDT_INTV) {
        clkCompUpdated += MIN_UPDT_INTV;
        CLK_COMP_setComp(clkCompRate);
    }
}

uint64_t CLK_COMP() {
    return CLK_COMP_fromMono(CLK_MONO());
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
}

int32_t CLK_COMP_getComp() {
    return clkCompRate;
}
