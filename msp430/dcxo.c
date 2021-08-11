
#include <msp430.h>
#include "dcxo.h"
#include "delay.h"
#include "i2c.h"

// flash constants
#define FLASH __attribute__ ((section(".text.const")))

// Si544 DCXO configuration strings
#ifdef __DCXO_SI544
// chip-select address
#define DCXO_CSA (0x55u)
// ADPLL register address
#define DCXO_REG_ADPLL (0xE7u)
// set register page
FLASH const uint8_t DCXO_REG_PAGE[] = { 0xFF, 0x00 };
// disable FCAL
FLASH const uint8_t DCXO_FCAL_OFF[] = { 0x45, 0x00 };
// disable output
FLASH const uint8_t DCXO_OUT_OFF [] = { 0x11, 0x00 };
// set LSDIV = 0 and HSDIV = 432
FLASH const uint8_t DCXO_HLDIV   [] = { 0x17, 0xB0, 0x01 };
// set FBDIV to 196.1852861 (0x00 46 C5 F4 97 A7)
FLASH const uint8_t DCXO_FBDIV   [] = { 0x1A, 0xFB, 0xE8, 0x6E, 0x2F, 0xC4, 0x00 };
// enable FCAL
FLASH const uint8_t DCXO_FCAL_ON [] = { 0x07, 0x08 };
// enable output
FLASH const uint8_t DCXO_OUT_ON  [] = { 0x11, 0x01 };
#endif

// Si549 DCXO configuration strings
#ifdef __DCXO_SI549
// chip-select address
#define DCXO_CSA (0x55u)
// ADPLL register address
#define DCXO_REG_ADPLL (0xE7u)
// set register page
FLASH const uint8_t DCXO_REG_PAGE[] = { 0xFF, 0x00 };
// disable FCAL
FLASH const uint8_t DCXO_FCAL_OFF[] = { 0x45, 0x00 };
// disable output
FLASH const uint8_t DCXO_OUT_OFF [] = { 0x11, 0x00 };
// set LSDIV = 0 and HSDIV = 432
FLASH const uint8_t DCXO_HLDIV   [] = { 0x17, 0xB0, 0x01 };
// set FBDIV to 70.77326343 (0x00 46 C5 F4 97 A7)
FLASH const uint8_t DCXO_FBDIV   [] = { 0x1A, 0xA7, 0x97, 0xF4, 0xC5, 0x46, 0x00 };
// enable FCAL
FLASH const uint8_t DCXO_FCAL_ON [] = { 0x07, 0x08 };
// enable output
FLASH const uint8_t DCXO_OUT_ON  [] = { 0x11, 0x01 };
#endif

// TMP117 configuration strings
#define TMPS_CSA (0x49u)
// ADPLL register address
#define TMPS_REG_RESULT (0x00u)
// set sensor configuration
FLASH const uint8_t TMPS_CONF[] = { 0x01, 0x00, 0x60 };

/**
 * Initialize I2C DCXO
 * Sets center frequncy to 25MHz and enables clock output
 */
void DCXO_init() {
    // send configuration sequence
    I2C_startWrite(DCXO_CSA);
    I2C_write(DCXO_REG_PAGE, sizeof(DCXO_REG_PAGE));
    I2C_stop();
    // send configuration sequence
    I2C_startWrite(DCXO_CSA);
    I2C_write(DCXO_FCAL_OFF, sizeof(DCXO_FCAL_OFF));
    I2C_stop();
    // send configuration sequence
    I2C_startWrite(DCXO_CSA);
    I2C_write(DCXO_OUT_OFF, sizeof(DCXO_OUT_OFF));
    I2C_stop();
    // send configuration sequence
    I2C_startWrite(DCXO_CSA);
    I2C_write(DCXO_HLDIV, sizeof(DCXO_HLDIV));
    I2C_stop();
    // send configuration sequence
    I2C_startWrite(DCXO_CSA);
    I2C_write(DCXO_FBDIV, sizeof(DCXO_FBDIV));
    I2C_stop();
    // send configuration sequence
    I2C_startWrite(DCXO_CSA);
    I2C_write(DCXO_FCAL_ON, sizeof(DCXO_FCAL_ON));
    I2C_stop();
    // send configuration sequence
    I2C_startWrite(DCXO_CSA);
    I2C_write(DCXO_OUT_ON, sizeof(DCXO_OUT_ON));
    I2C_stop();

    // Enable DCXO output (P1-0)
    P1SEL &= ~BIT0;
    P1OUT |=  BIT0;
    P1DIR |=  BIT0;
    P1REN &= ~BIT0;

    // configure temp sensor
    I2C_startWrite(TMPS_CSA);
    I2C_write(TMPS_CONF, sizeof(TMPS_CONF));
    I2C_stop();
}

/**
 * Fine adjustment of DCXO output frequency (24-bit resolution)
 * @param offset - frequency adjustment offset in 0.1164 ppb incrememts
 */
void DCXO_setOffset(int32_t offset) {
    I2C_startWrite(DCXO_CSA);
    I2C_write8(DCXO_REG_ADPLL);
    I2C_write(&offset, 3);
    I2C_stop();
}

/**
 * Get DCXO temperature from I2C sensor
 * return temperature in Celsis (8.8 fixed-point format)
 */
int16_t DCXO_getTemp() {
    // set register address
    I2C_startWrite(TMPS_CSA);
    I2C_write8(TMPS_REG_RESULT);
    // read measurement
    I2C_startRead(TMPS_CSA);
    int16_t result = I2C_read16b();
    I2C_stop();

    // check for bad measurement
    if(result == DCXO_TMPS_ERR)
        return DCXO_TMPS_ERR;

    // adjust alignment
    return result << 1u;
}
