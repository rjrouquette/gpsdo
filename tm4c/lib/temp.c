//
// Created by robert on 4/17/22.
//

#include "../hw/adc.h"
#include "delay.h"
#include "temp.h"
#include "led.h"

static volatile uint16_t adc_temp = 0;

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
}

int16_t TEMP_proc() {
    ADC0.PSSI.SS3 = 1;
    int32_t temp = adc_temp;
    temp *= 63360;
    temp = 154664960 - temp;
    return (int16_t) (temp / 4096);
}

void ISR_ADC0Sequence3(void) {
    // clear interrupt
    ADC0.ISC.IN3 = 1;
    // store value
    adc_temp = ADC0.SS3.FIFO.DATA;
}
