/**
 * GPSDO with NTP and IEEE PTP
 * @author Robert J. Rouquette
 * @date 2022-04-13
 */

#include "hw/gpio.h"
#include "lib/delay.h"
#include "lib/epaper.h"

int main(void) {
    EPAPER_init();

    RCGCGPIO.EN_PORTN = 1;
    delay_cycles_4();

    PORTN.LOCK = GPIO_LOCK_KEY;
    PORTN.CR = 0x01u;
    PORTN.AMSEL = 0;
    PORTN.PCTL = 0;
    PORTN.AFSEL = 0;
    PORTN.DR8R = 0x01u;
    PORTN.DIR = 0x01u;
    PORTN.DEN = 0x01u;
    PORTN.DATA[0x01u] = 0x01u;
    for(;;) {
        delay_loop(1u << 20u);
        PORTN.DATA[0x01u] ^= 0x01u;
    }
}

// ISR Test
void ISR_Timer0A() {
    __asm volatile("nop");
}
