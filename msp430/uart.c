
#include <msp430.h>
#include "uart.h"

#define FRAME_MAGIC (0x4ab7)

volatile uint8_t rxHead = 0;
volatile uint8_t rxTail = 0;
volatile uint8_t rxBuff[256];
volatile uint8_t txHead = 0;
volatile uint8_t txTail = 0;
volatile uint8_t txBuff[256];

void appendCRC(uint8_t *buff, uint16_t len);
uint16_t verifyCRC(uint8_t *buff, uint16_t len);

/**
 * Reset UART
 */
void UART_init() {
    // configure GPIO
    P1OUT &= ~(BIT1 | BIT2);
    P1REN &= ~(BIT1 | BIT2);
    P1DIR |= BIT1;
    P1DIR &= BIT2;
    P1SEL |= BIT1 | BIT2;

    // configure USCA0 (115200 baud w/ 25MHz)
    UCA0CTL1 |= UCSWRST;
    // prescaler
    UCA0BRW = 13u;
    // modulator
    UCA0MCTL = UCBRF_9 | UCBRS_0 | UCOS16;
    // release USCA0
    UCA0CTL1 &= ~UCSWRST;
    // enable RX interrupt
    UCA0IE |= UCRXIE;
}

/**
 * Poll UART interface
 */
void UART_poll() {

}

void appendCRC(uint8_t *buff, uint16_t len) {
    // init CRC
    CRCINIRES = 0xFFFFu;
    uint8_t *end = buff + len;
    while(buff < end) {
        CRCDI_L = *(buff++);
    }
    end[0] = CRCINIRES_L;
    end[1] = CRCINIRES_H;
}

uint16_t verifyCRC(uint8_t *buff, uint16_t len) {
    // init CRC
    CRCINIRES = 0xFFFFu;
    uint8_t *end = buff + len;
    while(buff < end) {
        CRCDI_L = *(buff++);
    }
    if(end[0] != CRCINIRES_L) return 1;
    if(end[1] != CRCINIRES_H) return 1;
    return 0;
}

// RX/TX on USCI_A0
__interrupt_vec(USCI_A0_VECTOR)
void USCI_A0_ISR() {
    switch(UCA0IV) {
        case UCRXIFG:
            rxBuff[rxHead] = UCA0RXBUF;
            ++rxHead;
            break;
        case UCTXIFG:
            UCA0TXBUF = txBuff[txTail];
            if(++txTail == txHead)
                UCA0IE &= UCTXIE;
            break;
    }
}
