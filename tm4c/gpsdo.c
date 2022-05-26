//
// Created by robert on 5/24/22.
//

#include "gpsdo.h"
#include "hw/emac.h"
#include "hw/gpio.h"
#include "hw/timer.h"
#include "lib/delay.h"

void initPPS() {
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

    // 1 Hz 10% duty cycle
    EMAC0.PPS0INTVL = 25000000-1;
    EMAC0.PPS0WIDTH = 12500000-1;
    // use command mode
    EMAC0.PPSCTRL.TRGMODS0 = 3;
    EMAC0.PPSCTRL.PPSEN0 = 1;
    // start zero-aligned pulse train
    EMAC0.TARGNANO = 0;
    EMAC0.TARGSEC = EMAC0.TIMSEC + 1;
    EMAC0.PPSCTRL.PPSCTRL = 2;
}

void initEdgeComp() {
    // Enable Timer 5
    RCGCTIMER.EN_GPTM5 = 1;
    delay_cycles_4();
    // Configure Timer 5 for capture mode
    GPTM5.CFG.GPTMCFG = 4;
    GPTM5.TAMR.MR = 0x3;
    GPTM5.TBMR.MR = 0x3;
    // edge-time mode
    GPTM5.TAMR.CMR = 1;
    GPTM5.TBMR.CMR = 1;
    GPTM5.CTL.TAEVENT = 1;
    GPTM5.CTL.TBEVENT = 1;
    // count up
    GPTM5.TAMR.CDIR = 1;
    GPTM5.TBMR.CDIR = 1;
    // disable overflow interrupt
    GPTM5.TAMR.CINTD = 0;
    GPTM5.TBMR.CINTD = 0;
    // full 24-bit count
    GPTM5.TAILR = 0xFFFF;
    GPTM5.TBILR = 0xFFFF;
    GPTM5.TAPR = 0xFF;
    GPTM5.TBPR = 0xFF;
    // interrupts
    GPTM5.IMR.CAE = 1;
    GPTM5.IMR.CBE = 1;
    GPTM0.TAMR.MIE = 1;
    GPTM0.TBMR.MIE = 1;
    // start timer
    GPTM5.CTL.TAEN = 1;
    GPTM5.CTL.TBEN = 1;
    // synchronize timers
    GPTM0.SYNC = (3 << 10);

    // configure capture pins
    RCGCGPIO.EN_PORTM = 1;
    delay_cycles_4();
    // unlock GPIO config
    PORTM.LOCK = GPIO_LOCK_KEY;
    PORTM.CR = 0xC0u;
    // configure pins;
    PORTM.PCTL.PMC6 = 3;
    PORTM.PCTL.PMC7 = 3;
    PORTM.AFSEL.ALT6 = 1;
    PORTM.AFSEL.ALT7 = 1;
    PORTM.DEN = 0xC0u;
    // lock GPIO config
    PORTM.CR = 0;
    PORTM.LOCK = 0;
}

void GPSDO_init() {
    initPPS();
    initEdgeComp();
}

void GPSDO_run() {
    // TODO implement GPSDO logic
}

int GPSDO_isLocked() {
    return 1;
}

int GPSDO_offsetNano() {
    return 0;
}

// capture rising edge of PPS output for calculating CLK_MONOTONIC offset
void ISR_Timer5A() {
    // TODO implement GPSDO logic
    GPTM5.ICR.CAE = 1;
}

// capture rising edge of GPS PPS for offset measurement
void ISR_Timer5B() {
    // TODO implement GPSDO logic
    GPTM5.ICR.CBE = 1;
}
