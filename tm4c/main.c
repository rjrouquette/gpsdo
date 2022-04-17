/**
 * GPSDO with NTP and IEEE PTP
 * @author Robert J. Rouquette
 * @date 2022-04-13
 */

#include <string.h>
#include "lib/clk.h"
#include "lib/delay.h"
#include "lib/epd.h"
#include "lib/font.h"
#include "lib/format.h"
#include "lib/led.h"

int main(void) {
    LED_init();
    CLK_init();
    EPD_init();
    FONT_drawText(2, 0, "Display Configured", FONT_ASCII_16, 0, 3, EPD_setPixel);
    EPD_refresh();

    LED0_ON();

    uint16_t refresh = 0;
    char temp[16];
    for(;;) {
        delay_ms(500);
        LED1_TGL();

        if((refresh++ & 0x1fu) == 0) {
            strcpy(temp, "0x");
            toHex(refresh, 4, '0', temp+5);
            temp[6] = 0;
            FONT_drawText(2, 16, temp, FONT_ASCII_16, 0, 3, EPD_setPixel);

            int end = toDec(refresh >> 5u, 0, '0', temp);
            temp[end] = 0;
            FONT_drawText(2, 32, temp, FONT_ASCII_16, 0, 3, EPD_setPixel);

            EPD_refresh();
        }
    }
}

// ISR Test
void ISR_Timer0A() {
    __asm volatile("nop");
}
