//
// Created by robert on 5/24/22.
//

#include "gpsdo.h"
#include "hw/emac.h"
#include "hw/gpio.h"
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

void GPSDO_init() {
    initPPS();
}

int GPSDO_isLocked() {
    return 0;
}

int GPSDO_offsetNano() {
    return 0;
}
