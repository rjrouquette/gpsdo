#include <msp430.h>
#include "math.h"
#include "pid.h"
#include "tcxo.h"

#define TIMER_TO_DCXO (43) // 5 ppb / 0.1164 ppb

void doTrackingUpdate();

int main() {
    // Stop watchdog timer
    WDTCTL = WDTPW | WDTHOLD;               

    // TODO bootsrap MCU
    // D = 1/4, P = 1/64, I = 1/2048
    PID_setCoeff(((int32_t)1) << 18, ((int32_t)-1) << 14, 11u);

    for(;;) {
        // check for PPS rising edge
        if(1)
            doTrackingUpdate();
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

        uint8_t allowUpdates = 1; //isGpsLocked();
        if(allowUpdates) {
            // TODO get actual PPS error
            int32_t ppsError = mult16s16s(10/*getTrackingError()*/, TIMER_TO_DCXO);
            // Update PID tracking loop
            pidComp = PID_update(ppsError);
        }
        // combine compensation values
        totalComp = pidComp + tempComp;
        //DCXO_applyOffset(totalComp);

        if(allowUpdates) {
            // adjust temperature compensation
            TCXO_updateCompensation(tempC, totalComp);
        }
}
