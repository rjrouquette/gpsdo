//
// Created by robert on 5/28/22.
//

#include "../hw/uart.h"
#include "delay.h"
#include "gps.h"

void GPS_init() {
    // enable UART 0
    RCGCUART.EN_UART0 = 1;
    delay_cycles_4();

}

void GPS_run() {

}
