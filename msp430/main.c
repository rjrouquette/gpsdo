#include <msp430.h>
#include "math.h"
#include "pid.h"
#include "pps.h"
#include "tcxo.h"

#define TIMER_TO_DCXO (43) // 5 ppb / 0.1164 ppb

// global configuration
struct {
    int32_t ppsErrorOffset;
} config;

void doTrackingUpdate();

int main() {
    // Stop watchdog timer
    WDTCTL = WDTPW | WDTHOLD;               

    // TODO bootstrap MCU

    // init code modules
    PPS_init();

    // D = 1/4, P = 1/64, I = 1/2048
    PID_setCoeff(((int32_t)1) << 18, ((int32_t)-1) << 14, 11u);

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
        int16_t tempC = 0x2a19;
        uint8_t allowTempUpdate = (tempC != 0x8000);
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
            ppsError += config.ppsErrorOffset;
            // Update PID tracking loop
            pidComp = PID_update(ppsError);
        }

        // combine compensation values
        totalComp = pidComp + tempComp;
        // apply frequency adjustment
        //DCXO_applyOffset(totalComp);

        if(allowTempUpdate) {
            // adjust temperature compensation
            TCXO_updateCompensation(tempC, totalComp);
        }
}
