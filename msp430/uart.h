/*
UART Interface
*/

#ifndef MSP430_UART
#define MSP430_UART

#include <stdint.h>

/**
 * Reset UART
 */
void UART_init();

/**
 * Poll UART interface
 */
void UART_poll();

#endif
