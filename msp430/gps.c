
#include <msp430.h>
#include "delay.h"
#include "gps.h"
#include "i2c.h"

#define RST_PORT_SEL  P2SEL
#define RST_PORT_OUT  P2OUT
#define RST_PORT_REN  P2REN
#define RST_PORT_DIR  P2DIR
#define RST_PIN       BIT2

#define GPS_CSA (0x42u)

// interal state
uint8_t hasFix = 0;
// receive buffer
uint8_t msgBuff[256];
uint16_t msgEnd = 0;
uint16_t stopChar = 0;
uint16_t stopByte = sizeof(msgBuff);

static void processMessage();

// GPS configuration strings
#define FLASH __attribute__ ((section(".text.const")))
// GNSS measurement rate
FLASH const uint8_t UBX_CFG_RATE[] = {
    0xB5, 0x62,             // sync
    0x06, 0x08, 0x06, 0x00, // header (class, id, length)
    0x32, 0x00,             // measuremnt interval (ms) 0x03E8 = 50
    0x14, 0x00,             // 20 measurements per fix
    0x01, 0x00,             // align to GPS time
    0x5B, 0x32              // Checksum
};
// PPS configuration
FLASH const uint8_t UBX_CFG_TP5[] = {
    0xB5, 0x62,             // sync
    0x06, 0x31, 0x20, 0x00, // header (class, id, length)
    0x00,                   // TIMEPULSE 0
    0x00,                   // Version 0
    0x00, 0x00,             // Reserved
    0x00, 0x00,             // antenna delay (ns)
    0x00, 0x00,             // RF group delay (ns)
    0x40, 0x42, 0x0F, 0x00, // PPS period us (no-lock) 0xF4240 = 1M
    0x40, 0x42, 0x0F, 0x00, // PPS period us (gps-lock) 0xF4240 = 1M
    0x20, 0xA1, 0x07, 0x00, // PPS len us (no-lock) 0x7A120 = 0.5M
    0x20, 0xA1, 0x07, 0x00, // PPS len us (gps-lock) 0x7A120 = 0.5M
    0x00, 0x00, 0x00, 0x00, // PPS edge delay
    0xF7, 0x00, 0x00, 0x00, // Flags
    0x00, 0x55              // Checksum
};

/**
 * Init GPS DDC interface
 */
void GPS_reset() {
    // configure reset pin
    RST_PORT_SEL &= RST_PIN;
    RST_PORT_OUT &= RST_PIN;
    RST_PORT_DIR |= RST_PIN;
    RST_PORT_REN &= RST_PIN;

    // hold reset low for ~1ms (25MHz)
    delayLoop(8333u);
    // release reset
    RST_PORT_OUT |= RST_PIN;

    // clear fix flag
    hasFix = 0;

    // send UBX command
    I2C_startWrite(GPS_CSA);
    I2C_write(UBX_CFG_RATE, sizeof(UBX_CFG_RATE));
    I2C_stop();
    // send UBX command
    I2C_startWrite(GPS_CSA);
    I2C_write(UBX_CFG_TP5, sizeof(UBX_CFG_TP5));
    I2C_stop();
}

/**
 * Poll GPS DDC interface
 */
void GPS_poll() {
    // set register address
    I2C_startWrite(GPS_CSA);
    I2C_write8(0xFFu);
    // start read
    I2C_startRead(GPS_CSA);
    // read pending data
    for(uint16_t i = 0; i < 64; i++) {
        // get next byte
        uint8_t byte = I2C_read8();
        // process byte
        if(msgEnd == 0) {
            // if EOF, exit
            if(byte == 0xFFu) break;
            // message is NEMA format
            if(byte == '$') {
                msgBuff[msgEnd++] = byte;
                stopChar = '\n';
            }
            // message is UBX format
            else if(byte == 0xB5u) {
                msgBuff[msgEnd++] = byte;
                stopChar = -1u;
            }
        }
        else {
            if(msgEnd == 5 && stopChar == -1u) {
                msgBuff[msgEnd++] = byte;
                stopByte = *(uint16_t*)(msgBuff+4);
                stopByte += 8;
            }
            if(msgEnd < sizeof(msgBuff)) {
                msgBuff[msgEnd++] = byte;
            }
            if(msgEnd >= stopByte || byte == stopChar) {
                processMessage();
                msgEnd = 0;
                stopByte = sizeof(msgBuff);
            }
        }
    }
    I2C_stop();
}

/**
 * Check if GPS has position fix
 * @return non-zero if GPS has position fix
 */
uint8_t GPS_isLocked() {
    return hasFix;
}

static void processMessage() {
    // TODO process GPS messages
}
