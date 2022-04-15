/**
 * GPSDO with NTP and IEEE PTP
 * @author Robert J. Rouquette
 * @date 2022-04-13
 */

#include "lib/clk.h"
#include "lib/delay.h"
#include "lib/epd.h"
#include "lib/led.h"

int main(void) {
    LED_init();
    CLK_init();
    EPD_init();

    for(;;) {
        delay_ms(500);
        LED1_TGL();
    }
}

// ISR Test
void ISR_Timer0A() {
    __asm volatile("nop");
}
