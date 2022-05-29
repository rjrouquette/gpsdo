//
// Created by robert on 5/28/22.
//

#include <memory.h>
#include "../hw/uart.h"
#include "delay.h"
#include "gps.h"

static uint8_t msgBuff[256];
static uint8_t rxBuff[256];
static uint8_t rxHead;
static uint8_t rxTail;
static int msgLen, ubxLen;

void processUBX(uint8_t *msg, int len);

void GPS_init() {
    // enable GPIO A
    RCGCGPIO.EN_PORTJ = 1;
    // enable UART 3
    RCGCUART.EN_UART3 = 1;
    delay_cycles_4();

    // configure pins J0-J1
    PORTJ.LOCK = GPIO_LOCK_KEY;
    PORTJ.CR = 0x03u;
    PORTJ.DIR = 0x02u;
    PORTJ.DR8R = 0x02u;
    PORTJ.AMSEL = 0x00;
    PORTJ.AFSEL.ALT0 = 1;
    PORTJ.AFSEL.ALT1 = 1;
    PORTJ.PCTL.PMC0 = 1;
    PORTJ.PCTL.PMC1 = 1;
    PORTJ.DEN = 0x03u;
    // lock GPIO config
    PORTJ.CR = 0;
    PORTJ.LOCK = 0;

    // configure UART 3
    UART3.CTL.UARTEN = 0;
    // baud divisor = 813.80208 = (125 MHz / (16 * 9600 baud))
    UART3.IBRD.DIVINT = 813;
    UART3.FBRD.DIVFRAC = 51;
    // 8-bit data
    UART3.LCRH.WLEN = 3;
    // enable FIFO
    UART3.LCRH.FEN = 1;
    // enable UART
    UART3.CTL.RXE = 1;
    UART3.CTL.TXE = 0;
    UART3.CTL.UARTEN = 1;
}

void GPS_run() {
    // read GPS serial data
    while(!UART3.FR.RXFE)
        rxBuff[rxHead++] = UART3.DR.DATA;
    // process messages
    while(rxTail != rxHead) {
        if(msgLen) {
            if(msgLen < 6) {
                msgBuff[msgLen++] = rxBuff[rxTail];
                // validate frame sync
                if(msgLen == 2 && msgBuff[1] != 0x62)
                    msgLen = 0;
            }
            else if(msgLen == 6) {
                ubxLen = *(uint16_t*)(msgBuff+4);
                ubxLen += 8;
                msgBuff[msgLen++] = rxBuff[rxTail];
            }
            else if(msgLen >= ubxLen) {
                processUBX(msgBuff, msgLen);
            }
        }
        else if(rxBuff[rxTail] == 0xB5) {
            msgBuff[msgLen++] = rxBuff[rxTail];
        }
        ++rxTail;
    }
}

int GPS_hasLock() {
    return 1;
}

volatile uint8_t debugStr[7][24];
void processUBX(uint8_t *msg, int len) {

}
