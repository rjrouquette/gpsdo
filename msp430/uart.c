
#include <msp430.h>
#include "uart.h"

static volatile uint8_t isTX = 0;
static volatile uint8_t length = 0;
static volatile uint8_t buffer[256];

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
 * @return length of message received
 */
uint8_t UART_poll() {
    // rx mode
    if(UCA0IE & UCRXIE) {
        // check if reception has begun
        if(length == 0) return 0;
        // check if reception is complete
        if(buffer[length - 1] == 0) {
            // disable RX interrupt
            UCA0IE &= UCRXIE;
            // return message length
            return length - 1;
        }
    }
    // tx mode
    else if(isTX) {
        // check if transmission is complete
        if(!(UCA0IE & UCTXIE)) {
            // clear TX flag
            isTX = 0;
            // reset buffer pointer
            length = 0;
            // clear interrupt flag
            UCA0IFG &= UCRXIFG;
            // enable RX interrupt
            UCA0IE |= UCRXIE;
        }
    }
    return 0;
}

/**
 * Access UART message buffer
 * @return pointer to UART message buffer
 */
char * UART_getMessage() {
    return (char *) buffer;
}

/**
 * Send message in UART buffer
 */
void UART_send() {
    // set TX flag
    isTX = 1;
    // set buffer pointer
    length = 1;
    // clear interrupt flag
    UCA0IFG &= UCTXIFG;
    // enable TX interrupt
    UCA0IE |= UCTXIE;
    // send first byte
    UCA0TXBUF = buffer[0];
}

// RX on USCI_A0
void USCI_A0_ISR_RX() {
    buffer[length++] = UCA0RXBUF;
}

// TX on USCI_A0
void USCI_A0_ISR_TX() {
    uint8_t byte = buffer[length++];
    UCA0TXBUF = byte;
    if(byte == 0)
        UCA0IE &= UCTXIE;
}

// RX/TX on USCI_A0
__interrupt_vec(USCI_A0_VECTOR)
void USCI_A0_ISR() {
    switch(UCA0IV) {
        case 0x02:
            USCI_A0_ISR_RX();
            return;
        case 0x04:
            USCI_A0_ISR_TX();
            return;
    }
}
