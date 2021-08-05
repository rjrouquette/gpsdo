#include <msp430.h>
#include "config.h"
#include "eeram.h"
#include "gps.h"
#include "i2c.h"
#include "math.h"
#include "pid.h"
#include "pps.h"
#include "tcxo.h"

#define TIMER_NS (5) // 5 ns
#define TEMP_PPS_TOL (100) // 100ns
#define PPM_SCALE (0x218DEF41l)

#define DCXO_CSA (0x55u)

#define TEMP_CSA (0x49u)
#define TEMP_ERR (0x8000)

// internal state
int32_t prevTempComp = TCXO_ERR;
int32_t tempComp = 0;
int32_t pidComp = 0;
int32_t totalComp = 0;

// stat accumulators
int64_t accAvgError = 0;
uint64_t accRmsError = 0;

// normalized stats
int32_t avgError = 0; // 24.8 fixed-point (ns)
int32_t rmsError = 0; // 24.8 fixed-point (ns)
int32_t ppmAdjust = 0; // 16.16 fixed-point (ppm)

void doTrackingUpdate(int16_t ppsDelay);

void DCXO_init();
void DCXO_setOffset(int32_t);

void TempSens_init();
int16_t TempSens_read();

void SysBoot();
void SysClk();


int main() {
    // bootstrap MCU
    SysBoot();
    I2C_init();
    DCXO_init();
    SysClk();
    _enable_interrupts();

    // init code modules
    EERAM_init();
    SysConf_load();
    PPS_init();
    PID_init();

    // pet watchdog
    WDTCTL |= WDTPW | WDTCNTCL;
    // release watchdog
    WDTCTL = WDTPW | (WDTCTL & 0x7F);

    // main processing loop
    for(;;) {
        // pet watchdog
        WDTCTL |= WDTPW | WDTCNTCL;
        // poll GPS data
        GPS_poll();
        // check for PPS measurement
        int16_t ppsDelay = PPS_poll();
        if(ppsDelay != PPS_IDLE) {
            doTrackingUpdate(ppsDelay);
        }
    }

    return 0;
}

void doTrackingUpdate(int16_t ppsDelay) {
        // TODO get actual temp
        int16_t tempC = TempSens_read();
        uint8_t allowTempUpdate = (tempC != TEMP_ERR);
        if(allowTempUpdate) {
            // adjust precision of temperature measurement 
            tempC <<= 1u;
            // get temperature compensation
            tempComp = TCXO_getCompensation(tempC);
            if(tempComp == TCXO_ERR) {
                if(prevTempComp == TCXO_ERR)
                    tempComp = 0;
                else
                    tempComp = prevTempComp;
            } else {
                if(prevTempComp == TCXO_ERR)
                    PID_clearIntegral();
                prevTempComp = tempComp;
            }
        }

        // update PPS tracking loop
        if(ppsDelay != PPS_NOLOCK) {
            // TODO get actual PPS error
            int32_t ppsError = mult16s16s(ppsDelay, TIMER_NS);
            ppsError += sysConf.ppsErrorOffset;
            // Update PID tracking loop
            pidComp = PID_update(ppsError);
            // Update average error (63s EMA)
            accAvgError -= accAvgError >> 5u;
            accAvgError += ppsError << 16u;
            // Update RMS error (63s EMA)
            accRmsError -= accRmsError >> 5u;
            accRmsError += (uint64_t)mult32s32s(ppsError, ppsError) << 16u;
            // wait for loop to settle before updating temperature compensation
            if(ppsError < -TEMP_PPS_TOL) allowTempUpdate = 0;
            if(ppsError >  TEMP_PPS_TOL) allowTempUpdate = 0;
        } else {
            allowTempUpdate = 0;
        }

        // combine compensation values
        totalComp = pidComp + tempComp;
        // apply frequency adjustment
        DCXO_setOffset(totalComp);

        if(allowTempUpdate) {
            // adjust temperature compensation
            TCXO_updateCompensation(tempC, totalComp);
        }

        // compute normalized dcxo offset in ppm (16.16 fixed-point)
        ppmAdjust = mult32s32s(totalComp, PPM_SCALE) >> 32u;
}

// DCXO configuration strings
#define FLASH __attribute__ ((section(".text.const")))
// set register page
FLASH const uint8_t DCXO_REG_PAGE[] = { 0xFF, 0x00 };
// disable FCAL
FLASH const uint8_t DCXO_FCAL_OFF[] = { 0x45, 0x00 };
// disable output
FLASH const uint8_t DCXO_OUT_OFF [] = { 0x11, 0x00 };
// set LSDIV = 0 and HSDIV = 432
FLASH const uint8_t DCXO_HLDIV   [] = { 0x17, 0xB0, 0x01 };
// set FBDIV to 70.77326343 (0x00 46 C5 F4 97 A7)
FLASH const uint8_t DCXO_FBDIV   [] = { 0x1A, 0xA7, 0x97, 0xF4, 0xC5, 0x46, 0x00 };
// enable FCAL
FLASH const uint8_t DCXO_FCAL_ON [] = { 0x07, 0x08 };
// enable output
FLASH const uint8_t DCXO_OUT_ON  [] = { 0x11, 0x01 };

void DCXO_init() {
    // send configuration sequence
    I2C_startWrite(DCXO_CSA);
    I2C_write(DCXO_REG_PAGE, sizeof(DCXO_REG_PAGE));
    I2C_stop();
    // send configuration sequence
    I2C_startWrite(DCXO_CSA);
    I2C_write(DCXO_FCAL_OFF, sizeof(DCXO_FCAL_OFF));
    I2C_stop();
    // send configuration sequence
    I2C_startWrite(DCXO_CSA);
    I2C_write(DCXO_OUT_OFF, sizeof(DCXO_OUT_OFF));
    I2C_stop();
    // send configuration sequence
    I2C_startWrite(DCXO_CSA);
    I2C_write(DCXO_HLDIV, sizeof(DCXO_HLDIV));
    I2C_stop();
    // send configuration sequence
    I2C_startWrite(DCXO_CSA);
    I2C_write(DCXO_FBDIV, sizeof(DCXO_FBDIV));
    I2C_stop();
    // send configuration sequence
    I2C_startWrite(DCXO_CSA);
    I2C_write(DCXO_FCAL_ON, sizeof(DCXO_FCAL_ON));
    I2C_stop();
    // send configuration sequence
    I2C_startWrite(DCXO_CSA);
    I2C_write(DCXO_OUT_ON, sizeof(DCXO_OUT_ON));
    I2C_stop();
}

void DCXO_setOffset(int32_t offset) {
    I2C_startWrite(DCXO_CSA);
    I2C_write8(0xE7);
    I2C_write(&offset, 3);
    I2C_stop();
}

void TempSens_init() {
    // conf bytes
    uint8_t conf[3] = { 0x01, 0x00, 0x60 };
    // configure temp sensor
    I2C_startWrite(TEMP_CSA);
    I2C_write(conf, 3);
    I2C_stop();
}

int16_t TempSens_read() {
    // set register address
    I2C_startWrite(TEMP_CSA);
    I2C_write8(0x00);
    // read measurement
    I2C_startRead(TEMP_CSA);
    int16_t result = I2C_read16b();
    I2C_stop();
    return result;
}

void SysBoot() {
    // Stop watchdog timer
    WDTCTL = WDTPW | WDTHOLD;
    // Switch FLL reference to REFO
    UCSCTL3 = SELREF__REFOCLK | FLLREFDIV__1;
    // clear pin assignments
    PASEL = 0; PBSEL = 0;
    PADIR = 0; PBDIR = 0;
    PAOUT = 0; PBOUT = 0;
    // enable pull-down on all pins
    PAREN = -1; PBREN = -1;
}

void SysClk() {
    // change XT1 to HF bypass mode
    UCSCTL6 = XT1BYPASS | XTS;
    // select XT1 function on pin
    PJSEL = BIT5;
    // set all dividers to 1
    UCSCTL5 = 0;
    // set ACLK to REFO, other clocks to XT1
    UCSCTL4 = SELA__REFOCLK | SELS__XT1CLK | SELM__XT1CLK;
    // disable DCO and FLL
    __bis_SR_register(SCG0);
    __bis_SR_register(SCG1);
    // set I2C clk to 400 kHz
    I2C_prescaler(6250u);
    // configure watchdog with 1s interval
    WDTCTL = WDTPW | WDTHOLD | WDTSSEL__ACLK | WDTCNTCL | WDTIS__32K;
}
