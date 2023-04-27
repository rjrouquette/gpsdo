/**
 * GPSDO with NTP and IEEE PTP
 * @author Robert J. Rouquette
 * @date 2022-04-13
 */

#include "hw/eeprom.h"
#include "hw/sys.h"
#include "lib/clk/clk.h"
#include "lib/delay.h"
#include "lib/gps.h"
#include "lib/led.h"
#include "lib/net.h"
#include "lib/rand.h"

#include "gitversion.h"
#include "gpsdo.h"
#include "ntp.h"
#include "ptp.h"
#include "snmp.h"
#include "status.h"

#define EEPROM_FORMAT (0x00000000)

static void EEPROM_init();

int main(void) {
    // enable FPU
    CPAC.CP10 = 3;
    CPAC.CP11 = 3;

    // initialize status LEDs
    LED_init();
    // initialize system clock
    CLK_init();
    // initialize RNG
    RAND_init();
    // initialize EEPROM
    EEPROM_init();
    // initialize GPSDO
    GPSDO_init();
    // initialize networking
    NET_init();
    NTP_init();
    PTP_init();
    SNMP_init();
    STATUS_init();

    // main loop
    for(;;) {
        CLK_run();
        GPS_run();
        GPSDO_run();
        LED_run();
        NET_run();
        NTP_run();
        PTP_run();
    }
}

void debugBlink(int cnt) {
    LED1_OFF();
    delay_ms(2000);
    while(cnt--) {
        LED1_TGL();
        delay_ms(250);
        LED1_TGL();
        delay_ms(250);
    }
}

void Fault_Hard() {
    debugBlink(2);
}

void Fault_Memory() {
    debugBlink(3);
}

void Fault_Bus() {
    debugBlink(4);
}

void Fault_Usage() {
    debugBlink(5);
}

static void EEPROM_init() {
    // enable EEPROM
    RCGCEEPROM.EN_EEPROM = 1;
    delay_us(1);
    EEPROM_wait();

    // verify eeprom format
    EEPROM_seek(0);
    uint32_t format = EEPROM_read();
    if(format != EEPROM_FORMAT) {
        // reformat EEPROM
        EEPROM_mass_erase();
        EEPROM_seek(0);
        EEPROM_write(EEPROM_FORMAT);
        EEPROM_write(VERSION_FW);
    }
}
