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
#include "lib/plot.h"
#include "lib/net.h"
#include "lib/temp.h"
#include "hw/emac.h"
#include "hw/sys.h"

int main(void) {
    char temp[32];
    // enable FPU
    CPAC.CP10 = 3;
    CPAC.CP11 = 3;

    // initialize status LEDs
    LED_init();
    // initialize system clock
    CLK_init();
    // initialize display
    EPD_init();
    FONT_drawText(0, 0, "Display Configured", FONT_ASCII_16, 0, 3);
    EPD_refresh();
    // initialize temperature sensor
    TEMP_init();
    TEMP_proc();
    // initialize networking
    NET_init();
    NET_getMacAddress(temp);
    FONT_drawText(16, 248, temp, FONT_ASCII_16, 0, 3);

    PLOT_setRect(0, 0, EPD_width()-1, 15, 3);
    PLOT_setLine(0, 15, EPD_width()-1, 15, 1);
    PLOT_setLine(0, 215, EPD_width()-1, 215, 1);

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

            end = toHMS(now, temp);
            temp[end] = 0;
            FONT_drawText(0, 0, temp, FONT_ASCII_16, 0, 3);

            end = toTemp(TEMP_proc(), temp);
            temp[end] = 0;
            FONT_drawText(EPD_width()-73, 0, temp, FONT_ASCII_16, 0, 3);

            end = toDec(EMAC0.RXCNTGB, 8, ' ', temp);
            temp[end] = 0;
            FONT_drawText(0, 96, temp, FONT_ASCII_16, 0, 3);

            end = toDec(EMAC0.MFBOC.raw, 8, ' ', temp);
            temp[end] = 0;
            FONT_drawText(0, 112, temp, FONT_ASCII_16, 0, 3);

            end = toHex(EMAC0.HOSRXDESC, 8, '0', temp);
            temp[end] = 0;
            FONT_drawText(0, 128, temp, FONT_ASCII_16, 0, 3);

            end = toHex(EMAC0.CFG.raw, 8, '0', temp);
            temp[end] = 0;
            FONT_drawText(0, 144, temp, FONT_ASCII_16, 0, 3);

            NET_getLinkStatus(temp);
            FONT_drawText(0, 216, "LINK:", FONT_ASCII_16, 0, 3);
            FONT_drawText(48, 216, temp, FONT_ASCII_16, 0, 3);

            NET_getIpAddress(temp);
            FONT_drawText(16, 232, temp, FONT_ASCII_16, 0, 3);

            EPD_refresh();
            next += 10;
        }
    }
}
