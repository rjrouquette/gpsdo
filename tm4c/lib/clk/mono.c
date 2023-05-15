//
// Created by robert on 4/26/23.
//

#include "../delay.h"
#include "../hw/emac.h"
#include "../hw/gpio.h"
#include "../hw/interrupts.h"
#include "../hw/sys.h"
#include "../hw/timer.h"
#include "mono.h"
#include "tai.h"
#include "comp.h"
#include "util.h"

#define INPUT_DELAY (2)

volatile uint32_t clkMonoInt = 0;
volatile uint32_t clkMonoOff = 0;
volatile uint32_t clkMonoEth = 0;
volatile uint32_t clkMonoPps = 0;

// pps edge capture state
volatile struct ClockEvent clkMonoPpsEvent;


void initClkMono() {
    // Enable Timer 0
    RCGCTIMER.EN_GPTM0 = 1;
    delay_cycles_4();
    // Configure Timer 0
    GPTM0.TAILR = -1;
    GPTM0.TAMATCHR = CLK_FREQ;
    GPTM0.TAMR.MR = 0x2;
    GPTM0.TAMR.CDIR = 1;
    GPTM0.TAMR.CINTD = 0;
    GPTM0.IMR.TAM = 1;
    GPTM0.TAMR.MIE = 1;
    // start timer
    GPTM0.CTL.TAEN = 1;
}

void initClkEth() {
    // configure PPS output pin
    RCGCGPIO.EN_PORTG = 1;
    delay_cycles_4();
    // unlock GPIO config
    PORTG.LOCK = GPIO_LOCK_KEY;
    PORTG.CR = 0x01u;
    // configure pins
    PORTG.DIR = 0x01u;
    PORTG.DR8R = 0x01u;
    PORTG.PCTL.PMC0 = 0x5;
    PORTG.AFSEL.ALT0 = 1;
    PORTG.DEN = 0x01u;
    // lock GPIO config
    PORTG.CR = 0;
    PORTG.LOCK = 0;

    // disable flash prefetch per errata
    FLASHCONF.FPFOFF = 1;
    // enable MAC and PHY clocks
    RCGCEMAC.EN0 = 1;
    RCGCEPHY.EN0 = 1;
    while(!PREMAC.RDY0);
    while(!PREPHY.RDY0);

    // disable timer interrupts
    EMAC0.IM.TS = 1;
    // enable PTP clock
    EMAC0.CC.PTPCEN = 1;
    // configure PTP
    EMAC0.TIMSTCTRL.ALLF = 1;
    EMAC0.TIMSTCTRL.DGTLBIN = 1;
    EMAC0.TIMSTCTRL.TSEN = 1;
    // 25MHz = 40ns
    EMAC0.SUBSECINC.SSINC = 40;
    // init timer
    EMAC0.TIMSECU = 0;
    EMAC0.TIMNANOU.VALUE = 0;
    // start timer
    EMAC0.TIMSTCTRL.TSINIT = 1;
    // use PPS free-running mode
    EMAC0.PPSCTRL.TRGMODS0 = 3;
    // start 1 Hz PPS output
    EMAC0.PPSCTRL.PPSEN0 = 0;
    EMAC0.PPSCTRL.PPSCTRL = 0;

    // re-enable flash prefetch per errata
    FLASHCONF.FPFOFF = 0;
}

static void initCaptureTimer(volatile struct GPTM_MAP *timer) {
    // configure timer for capture mode
    timer->CFG.GPTMCFG = 4;
    timer->TAMR.MR = 0x3;
    timer->TBMR.MR = 0x3;
    // edge-time mode
    timer->TAMR.CMR = 1;
    timer->TBMR.CMR = 1;
    // rising edge
    timer->CTL.TAEVENT = 0;
    timer->CTL.TBEVENT = 0;
    // count up
    timer->TAMR.CDIR = 1;
    timer->TBMR.CDIR = 1;
    // disable overflow interrupt
    timer->TAMR.CINTD = 0;
    timer->TBMR.CINTD = 0;
    // full count range
    timer->TAILR = -1;
    timer->TBILR = -1;
    // interrupts
    timer->IMR.CAE = 1;
    timer->IMR.CBE = 1;
    // start timer
    timer->CTL.TAEN = 1;
    timer->CTL.TBEN = 1;
}

void initClkSync() {
    // Enable Timer 4
    RCGCTIMER.EN_GPTM4 = 1;
    delay_cycles_4();
    initCaptureTimer(&GPTM4);
    // disable timer B interrupt
    GPTM4.IMR.CBE = 0;

    // Enable Timer 5
    RCGCTIMER.EN_GPTM5 = 1;
    delay_cycles_4();
    initCaptureTimer(&GPTM5);

    // synchronize timers with monotonic clock
    GPTM0.SYNC = 0x0F03;

    // configure capture pins
    RCGCGPIO.EN_PORTM = 1;
    delay_cycles_4();
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

// second boundary comparison
void ISR_Timer0A() {
    // increment counter
    clkMonoOff += CLK_FREQ;
    ++clkMonoInt;
    // set next second boundary
    GPTM0.TAMATCHR = clkMonoOff + CLK_FREQ;
    // clear interrupt flag
    GPTM0.ICR.TAM = 1;
}

// return integer part of current time
uint32_t CLK_MONO_INT() {
    return clkMonoInt;
}

// return current time as 32.32 fixed point value
uint64_t CLK_MONO() {
    // capture current time
    __disable_irq();
    uint32_t snapF = GPTM0.TAV.raw;
    uint32_t snapI = clkMonoInt;
    uint32_t snapO = clkMonoOff;
    __enable_irq();
    // convert to timestamp
    return fromClkMono(snapF, snapO, snapI);
}


// capture rising edge of PPS output for offset measurement
void ISR_Timer4A() {
    // snapshot edge time
    uint32_t timer = GPTM0.TAV.raw;
    uint32_t event = GPTM4.TAR.raw;
    // clear interrupt flag
    GPTM4.ICR.CAE = 1;
    // compute pps output time
    timer -= ((timer + INPUT_DELAY) - event) & 0xFFFF;
    clkMonoPps = timer;
}

// capture rising edge of ethernet PPS for offset measurement
void ISR_Timer5A() {
    // snapshot edge time
    uint32_t timer = GPTM0.TAV.raw;
    uint32_t event = GPTM5.TAR.raw;
    // clear interrupt flag
    GPTM5.ICR.CAE = 1;
    // compute ethernet clock offset
    timer -= ((timer + INPUT_DELAY) - event) & 0xFFFF;
    timer -= EMAC0.TIMSEC * CLK_FREQ;
    clkMonoEth = timer;
}

// capture rising edge of GPS PPS for offset measurement
void ISR_Timer5B() {
    // snapshot edge time
    uint32_t timer = GPTM0.TAV.raw;
    uint32_t event = GPTM5.TBR.raw;
    // clear interrupt flag
    GPTM5.ICR.CBE = 1;
    // update pps edge state
    timer -= ((timer + INPUT_DELAY) - event) & 0xFFFF;
    // monotonic clock state
    clkMonoPpsEvent.timer = timer;
    clkMonoPpsEvent.offset = clkMonoOff;
    clkMonoPpsEvent.integer = clkMonoInt;
    // compmed clock state
    clkMonoPpsEvent.compRate = clkCompRate;
    clkMonoPpsEvent.compRef = clkCompRef;
    clkMonoPpsEvent.compOff = clkCompOffset;
    // tai clock state
    clkMonoPpsEvent.taiRate = clkTaiRate;
    clkMonoPpsEvent.taiRef = clkTaiRef;
    clkMonoPpsEvent.taiOff = clkTaiOffset;
}
