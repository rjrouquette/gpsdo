/**
 * GPSDO with NTP and IEEE PTP
 * @author Robert J. Rouquette
 * @date 2022-04-13
 */

#include <memory.h>
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

#include "gpsdo.h"
#include "ntp.h"
#include "snmp.h"
#include "lib/net/dhcp.h"

volatile uint8_t debugHex[32];

void updateStatusBanner();
void updateStatusNetwork();

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

    // banner background fill
    PLOT_setRect(0, 0, EPD_width()-1, 15, 2);
    // separators
    PLOT_setLine(0, 15, EPD_width()-1, 15, 1);
    PLOT_setLine(0, 183, EPD_width()-1, 183, 1);
    PLOT_setLine(0, 215, EPD_width()-1, 215, 1);

    // packet count headers
    FONT_drawText(0, 184, "RX:", FONT_ASCII_16, 0, 3);
    FONT_drawText(0, 200, "TX:", FONT_ASCII_16, 0, 3);
    FONT_drawText(120, 184, "frames", FONT_ASCII_16, 0, 3);
    FONT_drawText(120, 200, "frames", FONT_ASCII_16, 0, 3);

    GPSDO_init();
    NTP_init();
    SNMP_init();

    uint32_t next = 0;
    int end;
    for(;;) {
        GPSDO_run();
        LED_run();
        NET_run();

        updateStatusBanner();
        updateStatusNetwork();

        uint32_t now = CLK_MONOTONIC_INT();
        int32_t diff = (int32_t) (now - next);
        if(diff >= 0) {
            for(int j = 0; j < 4; j++) {
                for (int i = 0; i < 8; i++) {
                    toHex(debugHex[j*8 + i], 2, '0', temp);
                    temp[2] = 0;
                    FONT_drawText(i * 20, (j+1)*16, temp, FONT_ASCII_16, 0, 3);
                }
            }

            EPD_refresh();
            next += 10;
        }
    }
}

void updateStatusBanner() {
    char temp[32];
    int end;

    end = toHMS(CLK_TAI_INT(), temp);
    memcpy(temp + end, " TAI", 4);
    temp[end+4] = 0;
    FONT_drawText(0, 0, temp, FONT_ASCII_16, 0, 2);

    end = toTemp(TEMP_proc(), temp);
    temp[end] = 0;
    FONT_drawText(EPD_width()-73, 0, temp, FONT_ASCII_16, 0, 2);
}

void updateStatusNetwork() {
    char temp[32];
    int end;

    FONT_drawText(0, 168, DHCP_hostname(), FONT_ASCII_16, 0, 3);

    // clear packet counts
    PLOT_setRect(32, 184, 112, 214, 3);
    // clear link status
    PLOT_setRect(0, 216, EPD_width()-1, 247, 3);

    // packets received
    end = toDec(EMAC0.RXCNTGB, 10, ' ', temp);
    temp[end] = 0;
    FONT_drawText(32, 184, temp, FONT_ASCII_16, 0, 3);

    // packets sent
    end = toDec(EMAC0.TXCNTGB, 10, ' ', temp);
    temp[end] = 0;
    FONT_drawText(32, 200, temp, FONT_ASCII_16, 0, 3);

    // link status
    NET_getLinkStatus(temp);
    FONT_drawText(0, 216, "LINK:", FONT_ASCII_16, 0, 3);
    FONT_drawText(48, 216, temp, FONT_ASCII_16, 0, 3);

    // current ip address
    NET_getIpAddress(temp);
    FONT_drawText(16, 232, temp, FONT_ASCII_16, 0, 3);
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
