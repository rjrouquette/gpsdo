/*
I2C EERAM API
*/

#ifndef MSP430_EERAM
#define MSP430_EERAM

#include <stdint.h>

/**
 * Init EERAM
**/
void EERAM_init();

/**
 * Reset EERAM
**/
void EERAM_reset();

/**
 * Read EERAM block
**/
void EERAM_read(uint16_t addr, void *data, uint16_t size);

/**
 * Write EERAM block
**/
void EERAM_write(uint16_t addr, const void *data, uint16_t size);

/**
 * Clear EERAM block
**/
void EERAM_bzero(uint16_t addr, uint16_t size);

#endif
