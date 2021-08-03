
#include <msp430.h>
#include "delay.h"
#include "i2c.h"


#define I2C_PORT_SEL  P1SEL
#define I2C_PORT_OUT  P1OUT
#define I2C_PORT_REN  P1REN
#define I2C_PORT_DIR  P1DIR
#define SDA_PIN       BIT4                  // UCB0SDA pin
#define SCL_PIN       BIT5                  // UCB0SCL pin

/**
 * Init I2C
**/
void I2C_init() {
    // Set software reset
    UCB0CTL1 |= UCSWRST;
    // Assign I2C pins to USCI_B0
    I2C_PORT_SEL |= SDA_PIN | SCL_PIN;
    // I2C Master, synchronous mode
    UCB0CTL0 = UCMST | UCMODE_3 | UCSYNC;     
    // Use SMCLK, TX mode, keep SW reset
    UCB0CTL1 = UCSSEL__SMCLK | UCTR | UCSWRST;
    // fSCL = SMCLK/12 = ~100kHz
    UCB0BR0 = 12u;
    UCB0BR1 = 0u;
    // Clear slave address
    UCB0I2CSA = 0u;
    // Clear own address
    UCB0I2COA = 0u;
    // Clear software reset
    UCB0CTL1 &= ~UCSWRST;
}

/**
 * Set I2C prescaler
 * @param div - prescaler value
 */
void I2C_prescaler(uint16_t div) {
    // Set software reset
    UCB0CTL1 |= UCSWRST;
    UCB0BR0 = div;
    UCB0BR1 = 0u;
    // Clear software reset
    UCB0CTL1 &= ~UCSWRST;
}

/**
 * Start read transaction
 * @param csa - chip select address
 */
void I2C_startRead(uint8_t csa) {
    // set slave address
    UCB0I2CSA = csa;
    // set transmit mode
    UCB0CTL1 &= ~UCTR;
    // clear RX interrupt flag
    UCB0IFG &= ~UCRXIFG;
    // send start condition
    UCB0CTL1 |= UCTXSTT;
    // wait for start condition
    while(UCB0CTL1 & UCTXSTT);
}

/**
 * Start write transaction
 * @param csa - chip select address
 */
void I2C_startWrite(uint8_t csa) {
    // set slave address
    UCB0I2CSA = csa;
    // set transmit mode
    UCB0CTL1 |= UCTR;
    // clear TX interrupt flag
    UCB0IFG &= ~UCTXIFG;
    // send start condition
    UCB0CTL1 |= UCTXSTT;
    // wait for start condition
    while(UCB0CTL1 & UCTXSTT);
}

/**
 * Read consecutive bytes
 * @param data - input buffer
 * @param size - number of bytes to read
 */
void I2C_read(void *data, uint16_t size) {
    uint8_t *ptr = (uint8_t *) data;
    uint8_t *end = ptr + size;
    while(ptr < end) {
        I2C_waitRX();
        *(ptr++) = UCB0RXBUF;
    }
}

/**
 * Read single byte
 * @return data
 */
uint8_t I2C_read8() {
    I2C_waitRX();
    return UCB0RXBUF;
}

/**
 * Read as big endian
 * @return data
 */
uint16_t I2C_read16b() {
    uint16_t result = 0u;
    I2C_waitRX();
    ((uint8_t*)result)[1] = UCB0RXBUF;
    I2C_waitRX();
    ((uint8_t*)result)[0] =  UCB0RXBUF;
    // return result
    return result;
}

/**
 * Write consecutive bytes
 * @param data - output buffer
 * @param size - number of bytes to read
 */
void I2C_write(const void *data, uint16_t size) {
    const uint8_t *ptr = (const uint8_t *) data;
    const uint8_t *end = ptr + size;
    while(ptr < end) {
        I2C_waitTX();
        UCB0TXBUF = *(ptr++);
    }
    I2C_waitTX();
}

/**
 * Write single byte
 * @param data - output buffer
 */
void I2C_write8(uint8_t data) {
    I2C_waitTX();
    UCB0TXBUF = data;
    I2C_waitTX();
}

/**
 * Write as big endian
 * @param data - output buffer
 */
void I2C_write16b(uint16_t data) {
    I2C_waitTX();
    UCB0TXBUF = ((uint8_t*)&data)[1];
    I2C_waitTX();
    UCB0TXBUF = ((uint8_t*)&data)[0];
    I2C_waitTX();
}

/**
 * Stop transaction
 */
void I2C_stop() {
    // send stop condition
    UCB0CTL1 |= UCTXSTP;
    // wait for stop condition
    while (UCB0CTL1 & UCTXSTP);
}
