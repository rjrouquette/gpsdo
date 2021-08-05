/*
GPS DDC (I2C) Interface
*/

#ifndef MSP430_GPS
#define MSP430_GPS

#include <stdint.h>

/**
 * Reset GPS
 */
void GPS_reset();

/**
 * Poll GPS DDC interface
 */
void GPS_poll();

/**
 * Check if GPS has position fix
 * @return non-zero if GPS has position fix
 */
uint8_t GPS_hasFix();

#endif
