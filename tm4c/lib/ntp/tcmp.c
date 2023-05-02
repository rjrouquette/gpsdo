//
// Created by robert on 4/30/23.
//

#include <math.h>
#include "../clk/mono.h"
#include "../delay.h"
#include "../format.h"
#include "../../hw/adc.h"
#include "../../hw/eeprom.h"
#include "../../hw/timer.h"
#include "tcmp.h"

#define TEMP_UPDT_INTV (122070) // 1024 Hz
#define TEMP_ALPHA (0x1p-8f)

#define TCMP_ALPHA (0x1p-14f)
#define TCMP_EEPROM_BLOCK (0x0010)
#define TCMP_SAVE_INTV (3600) // save state every hour
#define TCMP_UPDT_INTV (CLK_FREQ / 16)  // 16 Hz

static volatile uint32_t tempNext = 0;
static volatile float tempValue;

static volatile float tcmpBias;
static volatile float tcmpCoef;
static volatile float tcmpOff;
static volatile uint32_t tcmpNext = 0;
static volatile uint32_t tcmpSaved;
static volatile float tcmpValue;

__attribute__((always_inline))
static inline float toCelsius(int32_t adcValue) {
    return -0.0604248047f * (float) (adcValue - 2441);
}

void TCMP_init() {
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
    ADC0.ACTSS.ASEN3 = 1;

    // take and discard some initial measurements to allow for settling
    for(int i = 0; i < 16; i++) {
        // trigger temperature measurement
        ADC0.PSSI.SS3 = 1;
        // wait for data
        while(!ADC0.RIS.INR3);
        // clear flag
        ADC0.ISC.IN3 = 1;
        // read data
        while(!ADC0.SS3.FSTAT.EMPTY)
            tempValue = toCelsius(ADC0.SS3.FIFO.DATA);
    }

    // load temperature compensation parameters
    uint32_t word;
    EEPROM_seek(TCMP_EEPROM_BLOCK);
    word = EEPROM_read();
    if(word != -1u) {
        // set temperature coefficient
        tcmpCoef = *(float *) &word;
        // load temperature bias
        word = EEPROM_read();
        tcmpBias = *(float *) &word;
        // load correction offset
        word = EEPROM_read();
        tcmpOff = *(float *) &word;
    }
    // reset if invalid
    if(!isfinite(tcmpCoef) || !isfinite(tcmpBias) || !isfinite(tcmpOff)) {
        tcmpCoef = 0;
        tcmpBias = 0;
        tcmpOff = 0;
    }
    // update compensation value
    tcmpValue = tcmpOff + (tcmpCoef * (tempValue - tcmpBias));
}

void TCMP_run() {
    // poll for next temperature update trigger
    if((GPTM0.TAV.raw - tempNext) > 0) {
        // start next temperature measurement
        ADC0.PSSI.SS3 = 1;
        // set next update time
        tempNext += TEMP_UPDT_INTV;
        // update temperature data
        while(!ADC0.SS3.FSTAT.EMPTY) {
            float temp = toCelsius(ADC0.SS3.FIFO.DATA);
            tempValue += (temp - tempValue) * TEMP_ALPHA;
        }
    }

    // poll for next compensation update trigger
    if(((int32_t) (tcmpNext - GPTM0.TAV.raw)) <= 0) {
        // set next update time
        tcmpNext += TCMP_UPDT_INTV;
        // update compensation value
        tcmpValue = tcmpOff + (tcmpCoef * (tempValue - tcmpBias));
    }
}

float TCMP_temp() {
    return tempValue;
}

float TCMP_get() {
    return tcmpValue;
}

void TCMP_update(float target) {
    if(tcmpBias == 0 && tcmpOff == 0) {
        tcmpBias = tempValue;
        tcmpOff = target;
        return;
    }

    const float temp = tempValue;
    tcmpBias += (temp - tcmpBias) * TCMP_ALPHA;
    tcmpOff += (target - tcmpOff) * TCMP_ALPHA;

    float error = target;
    error -= tcmpOff;
    error -= tcmpCoef * (temp - tcmpBias);
    error *= TCMP_ALPHA;
    error *= temp - tcmpBias;
    tcmpCoef += error;

    const uint32_t now = CLK_MONO_INT();
    if(now - tcmpSaved > TCMP_SAVE_INTV) {
        tcmpSaved = now;
        // save temperature compensation parameters
        EEPROM_seek(TCMP_EEPROM_BLOCK);
        EEPROM_write(*(uint32_t *) &tcmpCoef);
        EEPROM_write(*(uint32_t *) &tcmpBias);
        EEPROM_write(*(uint32_t *) &tcmpOff);
    }
}

unsigned TCMP_status(char *buffer) {
    char tmp[32];
    char *end = buffer;

    end = append(end, "tcomp status:\n");

    tmp[fmtFloat(tempValue, 12, 4, tmp)] = 0;
    end = append(end, "  - temp:  " );
    end = append(end, tmp);
    end = append(end, " C\n");

    tmp[fmtFloat(tcmpBias, 12, 4, tmp)] = 0;
    end = append(end, "  - bias:  ");
    end = append(end, tmp);
    end = append(end, " C\n");

    tmp[fmtFloat(tcmpCoef * 1e6f, 12, 4, tmp)] = 0;
    end = append(end, "  - coef:  ");
    end = append(end, tmp);
    end = append(end, " ppm/C\n");

    tmp[fmtFloat(tcmpOff * 1e6f, 12, 4, tmp)] = 0;
    end = append(end, "  - offs:  ");
    end = append(end, tmp);
    end = append(end, " ppm\n");

    tmp[fmtFloat(tcmpValue * 1e6f, 12, 4, tmp)] = 0;
    end = append(end, "  - curr:  ");
    end = append(end, tmp);
    end = append(end, " ppm\n\n");

    return end - buffer;
}
