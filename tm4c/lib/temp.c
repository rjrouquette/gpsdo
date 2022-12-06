//
// Created by robert on 4/17/22.
//

#include "../hw/adc.h"
#include "delay.h"
#include "temp.h"

static volatile float dcxoTemp;

void TEMP_init() {
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

float TEMP_value() {
    return dcxoTemp;
}

void ISR_ADC0Sequence3(void) {
    // clear interrupt
    ADC0.ISC.IN3 = 1;
    // update temperature
    int32_t temp = 2441;
    temp -= ADC0.SS3.FIFO.DATA;
    float _temp = (float) temp;
    _temp *= 0.0604248047f;
    _temp -= dcxoTemp;
    _temp *= 0x1p-8f;
    dcxoTemp += _temp;
    // start next temperature measurement
    ADC0.PSSI.SS3 = 1;
}
