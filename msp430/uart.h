/*
UART Interface
*/

#ifndef MSP430_UART
#define MSP430_UART

#include <stdint.h>

/**
 * Initialize UART
 */
void UART_init();

/**
 * Poll UART interface
 * @return length of message received
 */
uint8_t UART_poll();

/**
 * Access UART message buffer
 * @return pointer to UART message buffer
 */
char * UART_getMessage();

/**
 * Send message in UART buffer
 */
void UART_send();

#endif
