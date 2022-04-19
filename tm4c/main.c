/**
 * GPSDO with NTP and IEEE PTP
 * @author Robert J. Rouquette
 * @date 2022-04-13
 */

#include "lib/clk.h"
#include "lib/delay.h"
#include "lib/epd.h"
#include "lib/font.h"
#include "lib/format.h"
#include "lib/led.h"
#include "lib/temp.h"

int main(void) {
    // initialize status LEDs
    LED_init();
    // initialize system clock
    CLK_init();
    // initialize display
    EPD_init();
    FONT_drawText(0, 0, "Display Configured", FONT_ASCII_16, 0, 3, EPD_setPixel);
    EPD_refresh();
    // initialize temperature sensor
    TEMP_init();
    TEMP_proc();

    LED0_ON();

    uint16_t refresh = 0;
    char temp[32];
    int end;
    for(;;) {
        delay_ms(500);
        LED1_TGL();

        if((refresh++ & 0x1fu) == 0) {
            temp[0] = '0'; temp[1] = 'x';
            end = toHex(refresh, 4, '0', temp+2);
            temp[2 + end] = 0;
            FONT_drawText(0, 16, temp, FONT_ASCII_16, 0, 3, EPD_setPixel);

            end = toDec(refresh >> 5u, 22, ' ', temp);
            temp[end] = 0;
            FONT_drawText(0, 32, temp, FONT_ASCII_16, 0, 3, EPD_setPixel);

            end = toTemp(TEMP_proc(), temp);
            temp[end] = 0;
            FONT_drawText(0, 48, temp, FONT_ASCII_16, 0, 3, EPD_setPixel);

            end = toHex(*((uint32_t *) 0xE000E100), 8, '0', temp);
            temp[end] = 0;
            FONT_drawText(0, 64, temp, FONT_ASCII_16, 0, 3, EPD_setPixel);

            EPD_refresh();
        }
    }
}
