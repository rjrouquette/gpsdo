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
#include "lib/net/ip.h"

volatile uint8_t debugMac[6];

extern uint32_t dhcpXID;
extern uint32_t dhcpLeaseExpire;

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

    PLOT_setRect(0, 0, EPD_width()-1, 15, 2);
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

        int32_t diff = (int32_t) (now - next);
        if(diff >= 0) {
            end = toHMS(now, temp);
            temp[end] = 0;
            FONT_drawText(0, 0, temp, FONT_ASCII_16, 0, 2);

            end = toTemp(TEMP_proc(), temp);
            temp[end] = 0;
            FONT_drawText(EPD_width()-73, 0, temp, FONT_ASCII_16, 0, 2);

            for(int i = 0; i < 6; i++) {
                toHex(debugMac[i], 2, '0', temp);
                temp[2] = 0;
                FONT_drawText(i * 20, 16, temp, FONT_ASCII_16, 0, 3);
            }

            end = toDec(EMAC0.RXCNTGB, 8, ' ', temp);
            temp[end] = 0;
            FONT_drawText(0, 96, temp, FONT_ASCII_16, 0, 3);

            end = toDec(EMAC0.TXCNTGB, 8, ' ', temp);
            temp[end] = 0;
            FONT_drawText(0, 112, temp, FONT_ASCII_16, 0, 3);

            end = toHex(ipSubnet, 8, '0', temp);
            temp[end] = 0;
            FONT_drawText(0, 128, temp, FONT_ASCII_16, 0, 3);

            end = toHex(ipGateway, 8, '0', temp);
            temp[end] = 0;
            FONT_drawText(0, 144, temp, FONT_ASCII_16, 0, 3);

            end = toHex(ipDNS, 8, '0', temp);
            temp[end] = 0;
            FONT_drawText(0, 160, temp, FONT_ASCII_16, 0, 3);

            NET_getLinkStatus(temp);
            FONT_drawText(0, 216, "LINK:", FONT_ASCII_16, 0, 3);
            FONT_drawText(48, 216, temp, FONT_ASCII_16, 0, 3);

            NET_getIpAddress(temp);
            PLOT_setRect(0, 232, EPD_width()-1, 247, 3);
            FONT_drawText(16, 232, temp, FONT_ASCII_16, 0, 3);

            EPD_refresh();
            next += 10;
        }
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
