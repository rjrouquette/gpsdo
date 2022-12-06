/**
 * Online-Learning Temperature Compensation
 * @author Robert J. Rouquette
 */

#include "hw/adc.h"
#include "lib/delay.h"
#include "tcomp.h"

#define ALPHA (0x1p-16f)

static volatile float currTemp;
static volatile float coeff, offset;
static volatile float mean[2];
static volatile int init = 1;


void ISR_ADC0Sequence3(void) {
    // clear interrupt
    ADC0.ISC.IN3 = 1;
    // update temperature
    int32_t temp = ADC0.SS3.FIFO.DATA;
    temp -= 2441;
    float _temp = (float) temp;
    _temp *= -0.0604248047f;
    _temp -= currTemp;
    _temp *= 0x1p-8f;
    currTemp += _temp;
    // start next temperature measurement
    ADC0.PSSI.SS3 = 1;
}

void TCOMP_init() {
    // Enable ADC0
    RCGCADC.EN_ADC0 = 1;
    delay_cycles_4();

    // configure ADC0 for temperature measurement
    ADC0.CC.CLKDIV = 0;
    ADC0.CC.CS = ADC_CLK_MOSC;
    ADC0.SAC.AVG = 6;
    ADC0.EMUX.EM3 = ADC_SS_TRIG_SOFT;
    ADC0.SS3.CTL.IE0 = 1;
    ADC0.SS3.CTL.END0 = 1;
    ADC0.SS3.CTL.TS0 = 1;
    ADC0.SS3.TSH.TSH0 = ADC_TSH_256;
    ADC0.IM.MASK3 = 1;
    ADC0.ACTSS.ASEN3 = 1;
    // trigger temperature measurement
    ADC0.PSSI.SS3 = 1;
}

float TCOMP_temperature() {
    return currTemp;
}

void TCOMP_updateTarget(float target) {
    float temp = currTemp;

    if(init) {
        init = 0;
        mean[0] = temp;
        mean[1] = target;
    }

    // update means
    mean[0] += ALPHA * (temp - mean[0]);
    mean[1] += ALPHA * (target - mean[1]);

    float x = temp - mean[0];
    float y = target - mean[1];

    float error = y;
    error -= coeff * x;
    error *= ALPHA;

    // update temperature coefficient
    coeff += error * x;
}

float TCOMP_getComp(float *m, float *b) {
    *m = coeff;
    *b = mean[1] - (coeff * mean[0]);
    return (coeff * (currTemp - mean[0])) + mean[1];
}
