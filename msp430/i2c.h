/*
I2C Interface API
*/

#ifndef MSP430_I2C
#define MSP430_I2C

#include <stdint.h>

/**
 * Init I2C
**/
void I2C_init();

/**
 * Set I2C prescaler
 * @param div - prescaler value
 */
void I2C_prescaler(uint16_t div);

/**
 * Start read transaction
 * @param csa - chip select address
 */
void I2C_startRead(uint8_t csa);

/**
 * Start write transaction
 * @param csa - chip select address
 */
void I2C_startWrite(uint8_t csa);

/**
 * Read consecutive bytes
 * @param data - input buffer
 * @param size - number of bytes to read
 */
void I2C_readBytes(void *data, uint16_t size);

/**
 * Write consecutive bytes
 * @param data - output buffer
 * @param size - number of bytes to read
 */
void I2C_writeBytes(const void *data, uint16_t size);

/**
 * Stop transaction
 */
void I2C_stop();

#endif
