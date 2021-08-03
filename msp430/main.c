#include <msp430.h>
#include "config.h"
#include "eeram.h"
#include "i2c.h"
#include "math.h"
#include "pid.h"
#include "pps.h"
#include "tcxo.h"

#define TIMER_TO_DCXO (43) // 5 ppb / 0.1164 ppb

#define DCXO_CSA (0x55u)

#define TEMP_CSA (0x49u)
#define TEMP_ERR (0x8000)

void doTrackingUpdate();

void DCXO_init();
void DCXO_setOffset(int32_t);

void TempSens_init();
int16_t TempSens_read();

void SysBoot();
void SysClk();


int main() {
    SysBoot();             

    // Init I2C interace
    I2C_init();
    // Init DCXO (25 MHz)
    DCXO_init();
    // Init system clock
    SysClk();
    // set I2C clk to 400 kHz
    I2C_prescaler(6250u);

    // init code modules
    EERAM_init();
    SysConf_load();
    PPS_init();
    PID_init();

    for(;;) {
        // check for PPS rising edge
        if(PPS_isReady()) {
            PPS_clearReady();
            doTrackingUpdate();
        }
        // GPS_pollState();
    }

    return 0;
}

int32_t prevTempComp = 0x8000000l;
int32_t tempComp = 0;
int32_t pidComp = 0;
int32_t totalComp = 0;
void doTrackingUpdate() {
        // TODO get actual temp
        int16_t tempC = TempSens_read();
        uint8_t allowTempUpdate = (tempC != TEMP_ERR);
        if(allowTempUpdate) {
            // adjust precision of temperature measurement 
            tempC <<= 1u;
            // get temperature compensation
            tempComp = TCXO_getCompensation(tempC);
            if(tempComp == 0x8000000l) {
                if(prevTempComp == 0x8000000l)
                    tempComp = 0;
                else
                    tempComp = prevTempComp;
            } else {
                if(prevTempComp == 0x8000000l)
                    PID_clearIntegral();
                prevTempComp = tempComp;
            }
        }

        // update GPS tracking loop
        uint8_t allowPidUpdate = 1; //GPS_isLocked();
        allowTempUpdate &= allowPidUpdate;
        if(allowPidUpdate) {
            // TODO get actual PPS error
            int32_t ppsError = mult16s16s(PPS_getDelta(), TIMER_TO_DCXO);
            ppsError += sysConf.ppsErrorOffset;
            // Update PID tracking loop
            pidComp = PID_update(ppsError);
        }

        // combine compensation values
        totalComp = pidComp + tempComp;
        // apply frequency adjustment
        DCXO_setOffset(totalComp);

        if(allowTempUpdate) {
            // adjust temperature compensation
            TCXO_updateCompensation(tempC, totalComp);
        }
}

void DCXO_init() {
    uint8_t tmp[8];

    // set register page
    tmp[0] = 0xFF;
    tmp[1] = 0x00;
    I2C_startWrite(DCXO_CSA);
    I2C_write(tmp, 2);
    I2C_stop();

    // disable FCAL
    tmp[0] = 0x45;
    tmp[1] = 0x00;
    I2C_startWrite(DCXO_CSA);
    I2C_write(tmp, 2);
    I2C_stop();

    // disable output
    tmp[0] = 0x11;
    tmp[1] = 0x00;
    I2C_startWrite(DCXO_CSA);
    I2C_write(tmp, 2);
    I2C_stop();

    // set LSDIV = 0 and HSDIV = 432
    tmp[0] = 0x17;
    tmp[1] = 0xB0;
    tmp[2] = 0x01;
    I2C_startWrite(DCXO_CSA);
    I2C_write(tmp, 3);
    I2C_stop();

    // set FBDIV to 70.77326343 (0x00 46 C5 F4 97 A7)
    tmp[0] = 0x1A;
    tmp[1] = 0xA7;
    tmp[2] = 0x97;
    tmp[3] = 0xF4;
    tmp[4] = 0xC5;
    tmp[5] = 0x46;
    tmp[6] = 0x00;
    I2C_startWrite(DCXO_CSA);
    I2C_write(tmp, 7);
    I2C_stop();

    // start FCAL
    tmp[0] = 0x07;
    tmp[1] = 0x08;
    I2C_startWrite(DCXO_CSA);
    I2C_write(tmp, 2);
    I2C_stop();

    // enable output
    tmp[0] = 0x11;
    tmp[1] = 0x00;
    I2C_startWrite(DCXO_CSA);
    I2C_write(tmp, 2);
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

    // configure internal OSC
}

void SysClk() {
    // switch clock to DCXO
}
