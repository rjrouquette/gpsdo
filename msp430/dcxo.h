/*
I2C DCXO API
*/

#ifndef MSP430_DCXO
#define MSP430_DCXO

#include <stdint.h>

// Indicates error with temperature sensor
#define DCXO_TMPS_ERR (0x8000)

// use Si544 DCXO configuration
#define __DCXO_SI544

/**
 * Initialize I2C DCXO
 * Sets center frequncy to 25MHz and enables clock output
 */
void DCXO_init();

/**
 * Fine adjustment of DCXO output frequency (24-bit resolution)
 * @param offset - frequency adjustment offset in 0.1164 ppb incrememts
 */
void DCXO_setOffset(int32_t offset);

/**
 * Get DCXO temperature from I2C sensor
 * return temperature in Celsis (8.8 fixed-point format)
 */
int16_t DCXO_getTemp();

#endif
