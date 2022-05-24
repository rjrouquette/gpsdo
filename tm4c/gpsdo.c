//
// Created by robert on 5/24/22.
//

#include "gpsdo.h"
#include "hw/emac.h"
#include "hw/gpio.h"
#include "lib/delay.h"
#include "lib/clk.h"
#include "lib/led.h"

static uint8_t edgeUpdate = 0;
static uint32_t edgeValue = 0;

void updateFracTAI(uint32_t clkRaw);

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
    // configure interrupt
    PORTG.IEV = 0x01u;
    PORTG.IM = 0x01u;
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

void GPSDO_run() {
    if(edgeUpdate) {
        edgeUpdate = 0;
        updateFracTAI(edgeValue);
    }
}

int GPSDO_isLocked() {
    return 1;
}

int GPSDO_offsetNano() {
    return 0;
}

// capture rising edge of PPS output for calculating CLK_MONOTONIC offset
void ISR_GPIOPortG() {
    edgeValue = CLK_MONOTONIC_RAW();
    edgeUpdate = 1;
    PORTG.ICR = 1;
}

void updateFracTAI(uint32_t clkRaw) {
    register union {
        struct {
            uint32_t fpart;
            uint32_t ipart;
        };
        uint64_t full;
    } result;

    // split fractional time into fraction and remainder
    result.ipart = clkRaw;
    result.ipart /= 1953125;
    result.fpart = clkRaw;
    result.fpart -= 1953125 * result.ipart;

    // compute twiddle for fractional conversion
    register uint32_t twiddle = result.fpart;
    twiddle *= 1524;
    twiddle >>= 16;
    // apply fractional coefficient
    result.fpart *= 2199;
    // apply fraction twiddle
    result.fpart += twiddle;
    // adjust bit alignment
    result.full >>= 6;
    // remove ISR delay of 400 ns
    result.fpart -= 1718;
    // update TAI alignment
    CLK_setFracTAI(-result.fpart);
}
