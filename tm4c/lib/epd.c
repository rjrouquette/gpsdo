/**
 * Driver Library for Waveshare 2.7 inch e-Paper HAT
 * @author Robert J. Rouquette
 * @date 2022-04-15
 */

#include "../hw/ssi.h"
#include "../lib/delay.h"
#include "epd.h"

// Display resolution
#define EPD_WIDTH       176
#define EPD_HEIGHT      264

// EPD2IN7 commands
#define PANEL_SETTING                               0x00
#define POWER_SETTING                               0x01
#define POWER_OFF                                   0x02
#define POWER_OFF_SEQUENCE_SETTING                  0x03
#define POWER_ON                                    0x04
#define POWER_ON_MEASURE                            0x05
#define BOOSTER_SOFT_START                          0x06
#define DEEP_SLEEP                                  0x07
#define DATA_START_TRANSMISSION_1                   0x10
#define DATA_STOP                                   0x11
#define DISPLAY_REFRESH                             0x12
#define DATA_START_TRANSMISSION_2                   0x13
#define PARTIAL_DATA_START_TRANSMISSION_1           0x14
#define PARTIAL_DATA_START_TRANSMISSION_2           0x15
#define PARTIAL_DISPLAY_REFRESH                     0x16
#define LUT_FOR_VCOM                                0x20
#define LUT_WHITE_TO_WHITE                          0x21
#define LUT_BLACK_TO_WHITE                          0x22
#define LUT_WHITE_TO_BLACK                          0x23
#define LUT_BLACK_TO_BLACK                          0x24
#define PLL_CONTROL                                 0x30
#define TEMPERATURE_SENSOR_COMMAND                  0x40
#define TEMPERATURE_SENSOR_CALIBRATION              0x41
#define TEMPERATURE_SENSOR_WRITE                    0x42
#define TEMPERATURE_SENSOR_READ                     0x43
#define VCOM_AND_DATA_INTERVAL_SETTING              0x50
#define LOW_POWER_DETECTION                         0x51
#define TCON_SETTING                                0x60
#define TCON_RESOLUTION                             0x61
#define SOURCE_AND_GATE_START_SETTING               0x62
#define GET_STATUS                                  0x71
#define AUTO_MEASURE_VCOM                           0x80
#define VCOM_VALUE                                  0x81
#define VCM_DC_SETTING_REGISTER                     0x82
#define PROGRAM_MODE                                0xA0
#define ACTIVE_PROGRAM                              0xA1
#define READ_OTP_DATA                               0xA2

static uint8_t canvas[(EPD_WIDTH * EPD_HEIGHT) / 4];

static void initSSI();
static void initEPD();

static void Reset(void);

int EPD_width() { return EPD_WIDTH; }
int EPD_height() { return EPD_HEIGHT; }

void EPD_init() {
    initSSI();
    initEPD();
}

static void initSSI() {
    // configure SSI3 interface as SPI Master
    // enable SSI3 module
    RCGCSSI.EN_SSI3 = 1;

    // enable GPIO port Q
    RCGCGPIO.EN_PORTQ = 1;
    delay_cycles_4();

    // configure pins 0-3
    PORTQ.LOCK = GPIO_LOCK_KEY;
    PORTQ.CR = 0x0fu;
    PORTQ.AMSEL = 0x0fu;
    PORTQ.PCTL = 0xEEEE;
    PORTQ.AFSEL = 0x00u;
    PORTQ.DR2R = 0x0fu;
    PORTQ.DEN = 0x0fu;

    // lock GPIO config
    PORTQ.CR = 0;
    PORTQ.LOCK = 0;

    // configure SSI3
    SSI3.CR1.raw = 0;
    SSI3.CR0.raw = 0;
    // 8-bit data
    SSI3.CR0.DSS = SSI_DSS_8_BIT;
    // 2x prescaler
    SSI3.CPSR.CPSDVSR = 2;
    // use system clock
    SSI3.CC.CS = SSI_CLK_SYS;
}

static void initEPD() {
    Reset();
//    SendCommand(POWER_SETTING);
//    SendData(0x03);                  // VDS_EN, VDG_EN
//    SendData(0x00);                  // VCOM_HV, VGHL_LV[1], VGHL_LV[0]
//    SendData(0x2b);                  // VDH
//    SendData(0x2b);                  // VDL
//    SendData(0x09);                  // VDHR
//    SendCommand(BOOSTER_SOFT_START);
//    SendData(0x07);
//    SendData(0x07);
//    SendData(0x17);
//    // Power optimization
//    SendCommand(0xF8);
//    SendData(0x60);
//    SendData(0xA5);
//    // Power optimization
//    SendCommand(0xF8);
//    SendData(0x89);
//    SendData(0xA5);
//    // Power optimization
//    SendCommand(0xF8);
//    SendData(0x90);
//    SendData(0x00);
//    // Power optimization
//    SendCommand(0xF8);
//    SendData(0x93);
//    SendData(0x2A);
//    // Power optimization
//    SendCommand(0xF8);
//    SendData(0xA0);
//    SendData(0xA5);
//    // Power optimization
//    SendCommand(0xF8);
//    SendData(0xA1);
//    SendData(0x00);
//    // Power optimization
//    SendCommand(0xF8);
//    SendData(0x73);
//    SendData(0x41);
//    SendCommand(PARTIAL_DISPLAY_REFRESH);
//    SendData(0x00);
//    SendCommand(POWER_ON);
//    WaitUntilIdle();
//
//    SendCommand(PANEL_SETTING);
//    SendData(0xAF);        //KW-BF   KWR-AF    BWROTP 0f
//    SendCommand(PLL_CONTROL);
//    SendData(0x3A);       //3A 100HZ   29 150Hz 39 200HZ    31 171HZ
//    SendCommand(VCM_DC_SETTING_REGISTER);
//    SendData(0x12);
}

void EPD_clear() {
    for(int i = 0; i < sizeof(canvas); i++)
        canvas[i] = 0xFF;
}

void EPD_refresh() {

//    SendCommand(DATA_START_TRANSMISSION_1);
//    //DelayMs(2);
//    for(int i = 0; i < width * height / 8; i++) {
//        SendData(0xFF);
//    }
//    //DelayMs(2);
//    SendCommand(DATA_START_TRANSMISSION_2);
//    //DelayMs(2);
//    for(int i = 0; i < width * height / 8; i++) {
//        SendData(0xFF);
//    }
//    // DelayMs(2);
//    SendCommand(DISPLAY_REFRESH);
//    //DelayMs(200);
//    WaitUntilIdle();
}

static void Reset(void) {
//     DigitalWrite(reset_pin, HIGH);
//     DelayMs(200);
//     DigitalWrite(reset_pin, LOW);
//     DelayMs(10);
//     DigitalWrite(reset_pin, HIGH);
//     DelayMs(200);
}
