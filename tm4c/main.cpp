/**
 * GPSDO with NTP and IEEE PTP
 * @author Robert J. Rouquette
 * @date 2022-04-13
 */

#include "gitversion.h"
#include "status.hpp"
#include "hw/eeprom.h"
#include "hw/interrupts.h"
#include "hw/sys.h"
#include "lib/delay.hpp"
#include "lib/gps.hpp"
#include "lib/led.hpp"
#include "lib/net.hpp"
#include "lib/rand.hpp"
#include "lib/run.hpp"
#include "lib/clk/clk.hpp"
#include "lib/ntp/ntp.hpp"
#include "lib/ptp/ptp.hpp"
#include "lib/snmp/snmp.hpp"

#define EEPROM_FORMAT (0x00000006)

static void EEPROM_init();

extern "C"
int main() {
    // enable FPU
    CPAC.CP10 = 3;
    CPAC.CP11 = 3;

    // initialize system clock
    CLK_initSys();
    // initialize task scheduler
    initScheduler();
    // initialize status LEDs
    LED_init();
    // initialize clock
    CLK_init();
    // initialize RNG
    RAND_init();
    // initialize EEPROM
    EEPROM_init();
    // initialize GPS
    GPS_init();
    // initialize networking
    NET_init();
    ntp::init();
    PTP_init();
    snmp::init();
    status::init();

    // run task scheduler
    runScheduler();
}

void Fault_Hard() {
    faultBlink(2, 1);
}

void Fault_Memory() {
    faultBlink(2, 2);
}

void Fault_Bus() {
    faultBlink(2, 3);
}

void Fault_Usage() {
    faultBlink(2, 4);
}

static void EEPROM_init() {
    // enable EEPROM
    RCGCEEPROM.EN_EEPROM = 1;
    delay_us(1);
    EEPROM_wait();

    // verify eeprom format
    EEPROM_seek(0);
    const uint32_t format = EEPROM_read();
    if (format != EEPROM_FORMAT) {
        // reformat EEPROM
        EEPROM_mass_erase();
        EEPROM_seek(0);
        EEPROM_write(EEPROM_FORMAT);
        EEPROM_write(VERSION_FW);
    }
}
