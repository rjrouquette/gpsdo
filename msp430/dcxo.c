
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
}

void DCXO_setOffset(int32_t offset) {
    I2C_startWrite(DCXO_CSA);
    I2C_write8(0xE7);
    I2C_write(&offset, 3);
    I2C_stop();
}
