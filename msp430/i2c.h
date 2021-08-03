/*
I2C Interface API
*/

#ifndef MSP430_I2C
#define MSP430_I2C

#include <stdint.h>

/**
 * Init I2C interface
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
void I2C_read(void *data, uint16_t size);

/**
 * Read single byte
 * @return data
 */
uint8_t I2C_read8();

/**
 * Read as big endian
 * @return data
 */
uint16_t I2C_read16b();

/**
 * Write consecutive bytes
 * @param data - output buffer
 * @param size - number of bytes to read
 */
void I2C_write(const void *data, uint16_t size);

/**
 * Write single byte
 * @param data - output buffer
 */
void I2C_write8(uint8_t data);

/**
 * Write as big endian
 * @param data - output buffer
 */
void I2C_write16b(uint16_t data);

/**
 * Stop transaction
 */
void I2C_stop();

/**
 * Wait for RX operation to complete
 */
inline void I2C_waitRX() {
    while(!(UCB0IFG & UCRXIFG));
    UCB0IFG &= ~UCRXIFG;
}

/**
 * Wait for TX operation to complete
 */
inline void I2C_waitTX() {
    while(!(UCB0IFG & UCTXIFG));
    UCB0IFG &= ~UCTXIFG;
}

#endif
