//
// Created by robert on 4/27/23.
//

#include "../../hw/gpio.h"
#include "../../hw/interrupts.h"
#include "../../hw/timer.h"
#include "../delay.h"
#include "../gps.h"
#include "mono.h"
#include "tai.h"
#include "comp.h"
#include "util.h"

#define MIN_UPDT_INTV (CLK_FREQ / 4) // 4 Hz

#define PPS_TIMER GPTM2
#define PPS_PORT PORTA
#define PPS_PIN (1<<4)
#define PPS_UPDT_INTV (CLK_FREQ / 64) // 64 Hz
#define PPS_INTV_HI (CLK_FREQ / 10) // 100 ms

volatile uint64_t clkTaiUtcOffset = 0;

static volatile uint32_t clkTaiUpdated = 0;
static volatile uint32_t clkTaiRem = 0;
volatile uint64_t clkTaiOffset = 0;
volatile uint64_t clkTaiRef = 0;
volatile int32_t clkTaiRate = 0;

static volatile uint32_t clkPpsUpdated = 0;
static volatile uint32_t clkPpsLow = CLK_FREQ - PPS_INTV_HI - 1;


// PPS output timer
void ISR_Timer2A() {
    // set next second boundary
    if(PPS_TIMER.TAMR.TCACT == 0x3) {
        // output is now high, schedule transition to low
        PPS_TIMER.TAMR.TCACT = 0x2;
        PPS_TIMER.TAILR = PPS_INTV_HI - 1;
    }
    else {
        // output is now low, schedule transition to high
        PPS_TIMER.TAMR.TCACT = 0x3;
        PPS_TIMER.TAILR = clkPpsLow;
    }
    // clear interrupt flag
    PPS_TIMER.ICR.TATO = 1;
}

void initClkTai() {
    // Enable Timer 2
    RCGCTIMER.EN_GPTM2 = 1;
    delay_cycles_4();
    // Configure Timer
    PPS_TIMER.TAILR = CLK_FREQ;
    PPS_TIMER.TAMR.MR = 0x2;
    PPS_TIMER.TAMR.CDIR = 1;
    // set CCP pin on timeout
    PPS_TIMER.TAMR.TCACT = 0x3;
    // enable interrupt
    PPS_TIMER.IMR.TATO = 1;
    // start timer
    PPS_TIMER.CTL.TAEN = 1;

    // enable CCP output on PA2
    RCGCGPIO.EN_PORTA = 1;
    delay_cycles_4();
    // unlock GPIO config
    PPS_PORT.LOCK = GPIO_LOCK_KEY;
    PPS_PORT.CR = PPS_PIN;
    // configure pins
    PPS_PORT.DIR |= PPS_PIN;
    PPS_PORT.DR8R |= PPS_PIN;
    PPS_PORT.PCTL.PMC4 = 3;
    PPS_PORT.AFSEL.ALT4 = 1;
    PPS_PORT.DEN |= PPS_PIN;
    // lock GPIO config
    PPS_PORT.CR = 0;
    PPS_PORT.LOCK = 0;

    // initialize UTC offset
    clkTaiUtcOffset = ((uint64_t) GPS_taiOffset()) << 32;
}

void runClkTai() {
    // periodically update alignment to prevent numerical overflow
    if((GPTM0.TAV.raw - clkTaiUpdated) >= MIN_UPDT_INTV) {
        clkTaiUpdated += MIN_UPDT_INTV;
        CLK_TAI_setTrim(clkTaiRate);
    }

    // periodically update PPS output alignment
    if((GPTM0.TAV.raw - clkPpsUpdated) >= PPS_UPDT_INTV) {
        clkPpsUpdated += PPS_UPDT_INTV;

        // update PPS output alignment
        union fixed_32_32 scratch;
        scratch.full = CLK_TAI();
        if(scratch.fpart >= (7u << 29)) {
            // compute next TAI second boundary
            scratch.fpart = 0;
            ++scratch.ipart;
            uint32_t rem = 0;
            // translate to compensated domain
            scratch.full -= clkTaiOffset;
            scratch.full += corrFrac(-clkTaiRate, scratch.full - clkTaiRef, &rem);
            // translate to monotonic domain
            scratch.full -= clkCompOffset;
            scratch.full += corrFrac(-clkCompRate, scratch.full - clkCompRef, &rem);
            // translate to raw timer ticks
            scratch.full *= CLK_FREQ;
            // compute PPS output interval
            int32_t delta = (int32_t) (scratch.ipart - clkMonoPps);
            while(delta < CLK_FREQ / 2) delta += CLK_FREQ;
            while(delta > CLK_FREQ * 2) delta -= CLK_FREQ;
            clkPpsLow = delta - PPS_INTV_HI - 1;
            // update PPS output interval
            __disable_irq();
            if(PPS_TIMER.TAMR.TCACT == 0x3)
                PPS_TIMER.TAILR = clkPpsLow;
            __enable_irq();
        }
    }
}

uint64_t CLK_TAI() {
    // get monotonic time
    uint64_t scratch = CLK_MONO();
    uint32_t rem = 0;
    // translate to compensated domain
    scratch += corrFrac(clkCompRate, scratch - clkCompRef, &rem);
    scratch += clkCompOffset;
    // translate to TAI domain
    scratch += corrFrac(clkTaiRate, scratch - clkTaiRef, &rem);
    scratch += clkTaiOffset;
    return scratch;
}

uint64_t CLK_TAI_fromMono(uint64_t ts) {
    ts = CLK_COMP_fromMono(ts);
    ts += corrValue(clkTaiRate, (int64_t) (ts - clkTaiRef));
    ts += clkTaiOffset;
    return ts;
}

void CLK_TAI_setTrim(int32_t comp) {
    // prepare update values
    const uint64_t now = CLK_COMP();
    const int32_t offset = corrFrac(clkTaiRate, now - clkTaiRef, &clkTaiRem);

    // apply update
    __disable_irq();
    clkTaiRef = now;
    clkTaiRate = comp;
    clkTaiOffset += offset;
    __enable_irq();
}

int32_t CLK_TAI_getTrim() {
    return clkTaiRate;
}

void CLK_TAI_adjust(int64_t delta) {
    __disable_irq();
    clkTaiOffset += delta;
    __enable_irq();
}
