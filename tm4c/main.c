/**
 * GPSDO with NTP and IEEE PTP
 * @author Robert J. Rouquette
 * @date 2022-04-13
 */

#include "lib/clk.h"
#include "lib/delay.h"
#include "lib/gps.h"
#include "lib/led.h"
#include "lib/net.h"
#include "hw/sys.h"

#include "gpsdo.h"
#include "ntp.h"
#include "ptp.h"
#include "snmp.h"
#include "status.h"

int main(void) {
    // enable FPU
    CPAC.CP10 = 3;
    CPAC.CP11 = 3;
    // enable EEPROM
    RCGCEEPROM.EN_EEPROM = 1;

    // initialize status LEDs
    LED_init();
    // initialize system clock
    CLK_init();
    // initialize networking
    NET_init();
    NTP_init();
    PTP_init();
    SNMP_init();
    STATUS_init();
    // initialize GPSDO
    GPSDO_init();

    // main loop
    for(;;) {
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
