//
// Created by robert on 4/15/22.
//

#include "../hw/gpio.h"
#include "../hw/sys.h"
#include "delay.h"
#include "led.h"

#define UPDATE_INTERVAL (1953125) // 64 Hz

static uint32_t nextUpdate = 0;
static uint8_t state = 0;
static uint8_t status = 0;

void LED_init() {
    // enable port
    RCGCGPIO.EN_PORTN = 1;
    delay_cycles_4();
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
}

void LED0_ON() { PORTN.DATA[0x01u] = 0x01u; }
void LED0_OFF() { PORTN.DATA[0x01u] = 0x00u; }
void LED0_TGL() { PORTN.DATA[0x01u] ^= 0x01u; }

void LED1_ON() { PORTN.DATA[0x02u] = 0x02u; }
void LED1_OFF() { PORTN.DATA[0x02u] = 0x00u; }
void LED1_TGL() { PORTN.DATA[0x02u] ^= 0x02u; }

void LED_run() {
    // wait for next update time
    if((nextUpdate - STCURRENT.CURRENT) & (1u << 23u))
        return;
    // set next update time
    nextUpdate += UPDATE_INTERVAL;
    // update led state
    if(((++state) & 0xF) == 0) {
        // set LED state
        PORTN.DATA[0x03u] = status;
        status = 0;
    }
}

void LED_act0() {
    status |= 0x01u;
}

void LED_act1() {
    status |= 0x02u;
}
