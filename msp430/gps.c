
#include <msp430.h>
#include "delay.h"
#include "gps.h"

#define RST_PORT_SEL  P2SEL
#define RST_PORT_OUT  P2OUT
#define RST_PORT_REN  P2REN
#define RST_PORT_DIR  P2DIR
#define RST_PIN       BIT2

#define GPS_CSA (0x42u)

// interal state
uint8_t hasFix = 0;

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
}

/**
 * Poll GPS DDC interface
 */
void GPS_poll() {

}

/**
 * Check if GPS has position fix
 * @return non-zero if GPS has position fix
 */
uint8_t GPS_isLocked() {
    return hasFix;
}
