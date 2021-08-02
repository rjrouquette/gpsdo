/*
System Configuration
*/

#ifndef MSP430_EERAM
#define MSP430_EERAM

#include <stdint.h>

#define EERAM_CSA0 (0xA1u)
#define EERAM_CSA1 (0xA3u)

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
void EERAM_read(uint8_t csa, uint16_t addr, void *data, uint16_t size);

/**
 * Write EERAM block
**/
void EERAM_write(uint8_t csa, uint16_t addr, const void *data, uint16_t size);

/**
 * Clear EERAM block
**/
void EERAM_bzero(uint8_t csa, uint16_t addr, uint16_t size);

#endif
