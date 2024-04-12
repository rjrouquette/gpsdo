//
// Created by robert on 4/15/22.
//

#include "led.hpp"

#include "delay.hpp"
#include "run.hpp"
#include "../hw/gpio.h"


static constexpr int FAULT_BLINK = 200;
static constexpr int FAULT_PAUSE = 1000;

static volatile uint8_t status = 0;

static void runLed(void *ref) {
    // set LED state
    PORTN.DATA[0x03u] = status;
    status = 0;
}

void LED_init() {
    // enable port
    RCGCGPIO.EN_PORTN = 1;
    delay::cycles(4);
    // unlock configuration
    PORTN.LOCK = GPIO_LOCK_KEY;
    PORTN.CR = 0x03u;
    // configure port
    PORTN.DR8R = 0x03u;
    PORTN.DIR = 0x03u;
    PORTN.DEN = 0x03u;
    PORTN.DATA[0x03u] = 0x00u;
    // lock configuration
    PORTN.CR = 0;
    PORTN.LOCK = 0;

    // schedule LED update to run at 16 Hz
    runSleep(RUN_SEC / 16, runLed, nullptr);
}

void LED0_ON() { PORTN.DATA[0x01u] = 0x01u; }
void LED0_OFF() { PORTN.DATA[0x01u] = 0x00u; }
void LED0_TGL() { PORTN.DATA[0x01u] ^= 0x01u; }

void LED1_ON() { PORTN.DATA[0x02u] = 0x02u; }
void LED1_OFF() { PORTN.DATA[0x02u] = 0x00u; }
void LED1_TGL() { PORTN.DATA[0x02u] ^= 0x02u; }

void LED_act0() {
    status |= 0x01u;
}

void LED_act1() {
    status |= 0x02u;
}

[[noreturn]]
void faultBlink(const int a, const int b) {
    for(;;) {
        LED0_OFF();
        LED1_OFF();
        auto c = a;
        while (c--) {
            delay::millis(FAULT_BLINK);
            LED0_ON();
            delay::millis(FAULT_BLINK);
            LED0_OFF();
        }
        delay::millis(FAULT_BLINK);
        c = b;
        while (c--) {
            delay::millis(FAULT_BLINK);
            LED1_ON();
            delay::millis(FAULT_BLINK);
            LED1_OFF();
        }
        delay::millis(FAULT_PAUSE);
    }
}
