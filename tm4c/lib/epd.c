/**
 * Driver Library for Waveshare 2.7 inch e-Paper HAT
 * @author Robert J. Rouquette
 * @date 2022-04-15
 */

#include "../hw/ssi.h"
#include "../lib/delay.h"
#include "../lib/led.h"
#include "epd.h"

// Display resolution
#define EPD_WIDTH       176
#define EPD_HEIGHT      264

static volatile uint8_t canvas[2][(EPD_WIDTH * EPD_HEIGHT) / 8];

static void initSSI();
static void initEPD();
static void setLUT();

static void Reset(void);
static void SendCommand(uint8_t byte);
static void SendByte(uint8_t byte);
static void SendBytes(uint32_t len, const volatile uint8_t *bytes);
static void WaitBusy();

int EPD_width() { return EPD_WIDTH; }
int EPD_height() { return EPD_HEIGHT; }

void EPD_init() {
    initSSI();
    initEPD();
    setLUT();

    EPD_clear();
    EPD_refresh();
}

static void initSSI() {
    // enable SSI3 module
    RCGCSSI.EN_SSI3 = 1;
    // enable GPIO ports
    RCGCGPIO.EN_PORTK = 1;
    RCGCGPIO.EN_PORTQ = 1;
    delay_cycles_4();

    // configure pins 0-2
    PORTQ.LOCK = GPIO_LOCK_KEY;
    PORTQ.CR = 0x07u;
    PORTQ.DIR = 0x07u;
    PORTQ.DR8R = 0x07u;
    PORTQ.AMSEL = 0x00;
    PORTQ.AFSEL = 0x07u;
    PORTQ.PCTL = 0x0EEE;
    PORTQ.DEN = 0x07u;
    // lock GPIO config
    PORTQ.CR = 0;
    PORTQ.LOCK = 0;

    // configure SSI3
    SSI3.CR1.raw = 0;
    SSI3.CR0.raw = 0;
    // 8-bit data
    SSI3.CR0.DSS = SSI_DSS_8_BIT;
    // 8x prescaler
    SSI3.CPSR.CPSDVSR = 5;
    // use system clock
    SSI3.CC.CS = SSI_CLK_SYS;
    // enable SSI
    SSI3.CR1.SSE = 1;

    // configure extra control lines
    PORTK.LOCK = GPIO_LOCK_KEY;
    PORTK.CR = 0x07u;
    PORTK.AMSEL = 0;
    PORTK.PCTL = 0;
    PORTK.AFSEL = 0;
    PORTK.DR8R = 0x07u;
    PORTK.PUR = 0x02u;
    PORTK.DIR = 0x03u;
    PORTK.DATA[0x03u] = 0x02u;
    PORTK.DEN = 0x07u;
    // lock GPIO config
    PORTK.CR = 0;
    PORTK.LOCK = 0;
}

static void initEPD() {
    Reset();
    SendCommand(0x01);			//POWER SETTING
    SendByte (0x03);
    SendByte (0x00);
    SendByte (0x2b);
    SendByte (0x2b);

    SendCommand(0x06);         //booster soft start
    SendByte (0x07);		//A
    SendByte (0x07);		//B
    SendByte (0x17);		//C

    SendCommand(0xF8);         //boost??
    SendByte (0x60);
    SendByte (0xA5);

    SendCommand(0xF8);         //boost??
    SendByte (0x89);
    SendByte (0xA5);

    SendCommand(0xF8);         //boost??
    SendByte (0x90);
    SendByte (0x00);

    SendCommand(0xF8);         //boost??
    SendByte (0x93);
    SendByte (0x2A);

    SendCommand(0xF8);         //boost??
    SendByte (0xa0);
    SendByte (0xa5);

    SendCommand(0xF8);         //boost??
    SendByte (0xa1);
    SendByte (0x00);

    SendCommand(0xF8);         //boost??
    SendByte (0x73);
    SendByte (0x41);

    SendCommand(0x16);
    SendByte(0x00);

    SendCommand(0x04);
    WaitBusy();

    SendCommand(0x00);			//panel setting
    SendByte(0xbf);		//KW-BF   KWR-AF	BWROTP 0f

    SendCommand(0x30);			//PLL setting
    SendByte (0x90);      	//100hz

    SendCommand(0x61);			//resolution setting
    SendByte (0x00);		//176
    SendByte (0xb0);
    SendByte (0x01);		//264
    SendByte (0x08);

    SendCommand(0x82);			//vcom_DC setting
    SendByte (0x12);

    SendCommand(0X50);			//VCOM AND DATA INTERVAL SETTING
    SendByte(0x97);
}

void EPD_clear() {
    volatile uint8_t *ptr = canvas[0];
    for(int i = 0; i < sizeof(canvas); i++)
        ptr[i] = 0xFF;
}

void EPD_refresh() {
    SendCommand(0x10);
    SendBytes(sizeof(canvas[0]), canvas[0]);
    SendCommand(0x13);
    SendBytes(sizeof(canvas[1]), canvas[1]);
    SendCommand(0x12);
    LED1_OFF();
    WaitBusy();
}

void EPD_setPixel(int x, int y, uint8_t color) {
    if (x < 0 || x >= EPD_WIDTH) return;
    if (y < 0 || y >= EPD_HEIGHT) return;
    uint32_t offset = x + (y * EPD_WIDTH);
    uint32_t byte = offset >> 3u;
    uint8_t mask = 0x80u >> (offset & 0x7u);

    // Set/Clear MSB
    if(color & 0x2u)
        canvas[0][byte] |= mask;
    else
        canvas[0][byte] &= ~mask;

    // Set/Clear LSB
    if(color & 0x1u)
        canvas[1][byte] |= mask;
    else
        canvas[1][byte] &= ~mask;
}

static void Reset(void) {
    PORTK.DATA[0x02] = 0x02;
    delay_ms(200);
    PORTK.DATA[0x02] = 0x00;
    delay_ms(2);
    PORTK.DATA[0x02] = 0x02;
    delay_ms(200);
}

static void SendCommand(uint8_t byte) {
    // LO level indicates command
    PORTK.DATA[0x01] = 0x00;
    // wait for room in FIFO
    while(!SSI3.SR.TNF);
    // transmit data
    SSI3.DR.DATA = byte;
    // wait for FIFO to empty
    while(!SSI3.SR.TFE);
    // wait for transmission to complete
    while(SSI3.SR.BSY);
}

static void SendByte(uint8_t byte) {
    SendBytes(1, &byte);
}

static void SendBytes(uint32_t len, const volatile uint8_t *bytes) {
    // HI level indicates command
    PORTK.DATA[0x01] = 0x01;
    for(uint32_t i = 0; i < len; i++) {
        // wait for room in FIFO
        while(!SSI3.SR.TNF);
        // transmit data
        SSI3.DR.DATA = bytes[i];
    }
    // wait for FIFO to empty
    while(!SSI3.SR.TFE);
    // wait for transmission to complete
    while(SSI3.SR.BSY);
}

static void WaitBusy() {
    for(;;) {
        SendCommand(0x71);
        if(PORTK.DATA[0x04])
            break;
    }
}

//////////////////////////////////////full screen update LUT////////////////////////////////////////////
//0~3 gray
static const uint8_t EPD_2in7_gray_lut_vcom[] = {
        0x00	,0x00,
        0x00	,0x0A	,0x00	,0x00	,0x00	,0x01,
        0x60	,0x14	,0x14	,0x00	,0x00	,0x01,
        0x00	,0x14	,0x00	,0x00	,0x00	,0x01,
        0x00	,0x13	,0x0A	,0x01	,0x00	,0x01,
        0x00	,0x00	,0x00	,0x00	,0x00	,0x00,
        0x00	,0x00	,0x00	,0x00	,0x00	,0x00,
        0x00	,0x00	,0x00	,0x00	,0x00	,0x00,
};
//R21
static const uint8_t EPD_2in7_gray_lut_ww[] = {
        0x40	,0x0A	,0x00	,0x00	,0x00	,0x01,
        0x90	,0x14	,0x14	,0x00	,0x00	,0x01,
        0x10	,0x14	,0x0A	,0x00	,0x00	,0x01,
        0xA0	,0x13	,0x01	,0x00	,0x00	,0x01,
        0x00	,0x00	,0x00	,0x00	,0x00	,0x00,
        0x00	,0x00	,0x00	,0x00	,0x00	,0x00,
        0x00	,0x00	,0x00	,0x00	,0x00	,0x00,
};
//R22H	r
static const uint8_t EPD_2in7_gray_lut_bw[] = {
        0x40	,0x0A	,0x00	,0x00	,0x00	,0x01,
        0x90	,0x14	,0x14	,0x00	,0x00	,0x01,
        0x00	,0x14	,0x0A	,0x00	,0x00	,0x01,
        0x99	,0x0C	,0x01	,0x03	,0x04	,0x01,
        0x00	,0x00	,0x00	,0x00	,0x00	,0x00,
        0x00	,0x00	,0x00	,0x00	,0x00	,0x00,
        0x00	,0x00	,0x00	,0x00	,0x00	,0x00,
};
//R23H	w
static const uint8_t EPD_2in7_gray_lut_wb[] = {
        0x40	,0x0A	,0x00	,0x00	,0x00	,0x01,
        0x90	,0x14	,0x14	,0x00	,0x00	,0x01,
        0x00	,0x14	,0x0A	,0x00	,0x00	,0x01,
        0x99	,0x0B	,0x04	,0x04	,0x01	,0x01,
        0x00	,0x00	,0x00	,0x00	,0x00	,0x00,
        0x00	,0x00	,0x00	,0x00	,0x00	,0x00,
        0x00	,0x00	,0x00	,0x00	,0x00	,0x00,
};
//R24H	b
static const uint8_t EPD_2in7_gray_lut_bb[] = {
        0x80	,0x0A	,0x00	,0x00	,0x00	,0x01,
        0x90	,0x14	,0x14	,0x00	,0x00	,0x01,
        0x20	,0x14	,0x0A	,0x00	,0x00	,0x01,
        0x50	,0x13	,0x01	,0x00	,0x00	,0x01,
        0x00	,0x00	,0x00	,0x00	,0x00	,0x00,
        0x00	,0x00	,0x00	,0x00	,0x00	,0x00,
        0x00	,0x00	,0x00	,0x00	,0x00	,0x00,
};

static void setLUT() {
    SendCommand(0x20);
    SendBytes(sizeof(EPD_2in7_gray_lut_vcom), EPD_2in7_gray_lut_vcom);

    SendCommand(0x21);
    SendBytes(sizeof(EPD_2in7_gray_lut_ww), EPD_2in7_gray_lut_ww);

    SendCommand(0x22);
    SendBytes(sizeof(EPD_2in7_gray_lut_bw), EPD_2in7_gray_lut_bw);

    SendCommand(0x23);
    SendBytes(sizeof(EPD_2in7_gray_lut_wb), EPD_2in7_gray_lut_wb);

    SendCommand(0x24);
    SendBytes(sizeof(EPD_2in7_gray_lut_bb), EPD_2in7_gray_lut_bb);

    SendCommand(0x25);             //vcom
    SendBytes(sizeof(EPD_2in7_gray_lut_ww), EPD_2in7_gray_lut_ww);
}
