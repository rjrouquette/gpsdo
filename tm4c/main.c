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
#include "lib/net.h"
#include "lib/temp.h"
#include "hw/emac.h"

int main(void) {
    char temp[32];

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
    // initialize networking
    NET_init();
    NET_getMacAddress(temp);
    FONT_drawText(16, 248, temp, FONT_ASCII_16, 0, 3, EPD_setPixel);

    LED0_ON();

    uint32_t next = 0;
    int end;
    for(;;) {
        NET_poll();

        uint32_t now = CLK_MONOTONIC_INT();
        if(now & 0x1u)
            LED1_ON();
        else
            LED1_OFF();

        int32_t diff = now - next;
        if(diff >= 0) {
            uint64_t mono = CLK_MONOTONIC();

            end = toTemp(TEMP_proc(), temp);
            temp[end] = 0;
            FONT_drawText(0, 32, temp, FONT_ASCII_16, 0, 3, EPD_setPixel);

            end = toDec(now, 8, ' ', temp);
            temp[end] = 0;
            FONT_drawText(0, 48, temp, FONT_ASCII_16, 0, 3, EPD_setPixel);

            end = toDec(mono >> 32u, 8, ' ', temp);
            temp[end] = '.';
            end = toDec((1000 * ((mono >> 16u) & 0xFFFFu)) >> 16u, 3, '0', temp+9);
            temp[9+end] = 0;
            FONT_drawText(0, 64, temp, FONT_ASCII_16, 0, 3, EPD_setPixel);

            end = toDec(EMAC0.RXCNTGB, 8, ' ', temp);
            temp[end] = 0;
            FONT_drawText(0, 96, temp, FONT_ASCII_16, 0, 3, EPD_setPixel);

            end = toDec(EMAC0.MFBOC.raw, 8, ' ', temp);
            temp[end] = 0;
            FONT_drawText(0, 112, temp, FONT_ASCII_16, 0, 3, EPD_setPixel);

            end = toHex(EMAC0.HOSRXDESC, 8, '0', temp);
            temp[end] = 0;
            FONT_drawText(0, 128, temp, FONT_ASCII_16, 0, 3, EPD_setPixel);

            end = toHex(EMAC0.CFG.raw, 8, '0', temp);
            temp[end] = 0;
            FONT_drawText(0, 144, temp, FONT_ASCII_16, 0, 3, EPD_setPixel);

            NET_getLinkStatus(temp);
            FONT_drawText(0, 216, temp, FONT_ASCII_16, 0, 3, EPD_setPixel);

            NET_getIpAddress(temp);
            FONT_drawText(16, 232, temp, FONT_ASCII_16, 0, 3, EPD_setPixel);

            EPD_refresh();
            next += 10;
        }
    }
}
