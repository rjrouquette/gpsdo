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
#define TCMP_EEPROM_BASE (0x0010)
#define TCMP_SAVE_INTV (3600) // save state every hour
#define TCMP_UPDT_INTV (CLK_FREQ / 16)  // 16 Hz

#define SOM_EEPROM_BASE (0x0020)
#define SOM_NODE_CNT (64)
#define SOM_ALPHA (0x1p-10f)

static volatile uint32_t tempNext = 0;
static volatile float tempValue;

static volatile float tcmpBias;
static volatile float tcmpCoef;
static volatile float tcmpOff;
static volatile uint32_t tcmpNext = 0;
static volatile uint32_t tcmpSaved;
static volatile float tcmpValue;

// SOM for filtering compensation samples
static volatile float somComp[SOM_NODE_CNT][2];

// neighbor weight = exp(-2.0f * dist)
static const float somNW[SOM_NODE_CNT] = {
        1.0f,
        1.35335283e-1f,
        1.83156389e-2f,
        2.47875218e-3f,
        3.35462628e-4f,
        4.53999298e-05f,
        6.14421235e-06f,
        8.31528719e-07f,
        1.12535175e-07f,
        1.52299797e-08f,
        2.06115362e-09f,
        2.78946809e-10f,
        3.77513454e-11f,
        5.10908903e-12f,
        6.91440011e-13f,
        9.35762297e-14f,
        1.26641655e-14f,
        1.71390843e-15f,
        2.31952283e-16f,
        3.13913279e-17f,
        4.24835426e-18f,
        5.74952226e-19f,
        7.78113224e-20f,
        1.05306176e-20f,
        1.42516408e-21f,
        1.92874985e-22f,
        2.61027907e-23f,
        3.53262857e-24f,
        4.78089288e-25f,
        6.47023493e-26f,
        8.75651076e-27f,
        1.18506486e-27f,
        1.60381089e-28f,
        2.17052201e-29f,
        2.93748211e-30f,
        3.97544974e-31f,
        5.38018616e-32f,
        7.28129018e-33f,
        9.85415469e-34f,
        1.33361482e-34f,
        1.80485139e-35f,
        2.44260074e-36f,
        3.30570063e-37f,
        4.47377931e-38f,
        6.05460190e-39f,
        8.19401262e-40f,
        1.10893902e-40f,
        1.50078576e-41f,
        2.03109266e-42f,
        2.74878501e-43f,
        3.72007598e-44f,
        5.03457536e-45f,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0
};

__attribute__((always_inline))
static inline float toCelsius(int32_t adcValue) {
    return -0.0604248047f * (float) (adcValue - 2441);
}

static void loadSom();
static void saveSom();
static void seedSom(float temp, float comp);
static void updateSom(float temp, float comp);


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

    // load temperature compensation parameters
    uint32_t word;
    EEPROM_seek(TCMP_EEPROM_BASE);
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

    // take and discard some initial measurements
    for(int i = 0; i < 16; i++) {
        ADC0.PSSI.SS3 = 1;      // trigger temperature measurement
        while(!ADC0.RIS.INR3);  // wait for data
        ADC0.ISC.IN3 = 1;       // clear flag
        // drain FIFO
        while(!ADC0.SS3.FSTAT.EMPTY)
            tempValue = toCelsius(ADC0.SS3.FIFO.DATA);
    }
    // set compensation value
    tcmpValue = tcmpOff + (tcmpCoef * (tempValue - tcmpBias));

    loadSom();
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
    if((GPTM0.TAV.raw - tcmpNext) > 0) {
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

void TCMP_update(const float target) {
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

    updateSom(temp, target);

    const uint32_t now = CLK_MONO_INT();
    if(now - tcmpSaved > TCMP_SAVE_INTV) {
        tcmpSaved = now;
        // save temperature compensation parameters
        EEPROM_seek(TCMP_EEPROM_BASE);
        EEPROM_write(*(uint32_t *) &tcmpCoef);
        EEPROM_write(*(uint32_t *) &tcmpBias);
        EEPROM_write(*(uint32_t *) &tcmpOff);

        saveSom();
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

static void loadSom() {
    uint32_t *ptr = (uint32_t *) somComp;
    uint32_t *end = ptr + (sizeof(somComp) / sizeof(uint32_t));

    // load SOM data
    EEPROM_seek(SOM_EEPROM_BASE);
    while(ptr < end)
        *(ptr++) = EEPROM_read();
}

static void saveSom() {
    uint32_t *ptr = (uint32_t *) somComp;
    uint32_t *end = ptr + (sizeof(somComp) / sizeof(uint32_t));

    // load SOM data
    EEPROM_seek(SOM_EEPROM_BASE);
    while(ptr < end)
        EEPROM_write(*(ptr++));
}

static void seedSom(float temp, float comp) {
    float norm = 1.0f / (float) SOM_NODE_CNT;
    int mid = SOM_NODE_CNT / 2;
    for(int i = 0; i < SOM_NODE_CNT; i++) {
        somComp[i][0] = temp + (norm * (float)(i - mid));
        somComp[i][1] = comp;
    }
}

static void updateSom(float temp, float comp) {
    if(!isfinite(somComp[0][0])) {
        seedSom(temp, comp);
        return;
    }

    // locate closest node
    int best = 0;
    float dist = fabsf(temp - somComp[0][0]);
    for(int i = 1; i < SOM_NODE_CNT; i++) {
        float diff = fabsf(temp - somComp[i][0]);
        if(diff < dist) {
            dist = diff;
            best = i;
        }
    }

    // update nodes
    for(int i = 0; i < SOM_NODE_CNT; i++) {
        // compute neighbor distance
        int ndist = i - best;
        if(ndist < 0) ndist = -ndist;
        // compute node alpha
        const float alpha = SOM_ALPHA * somNW[ndist];
        // update node weights
        somComp[i][0] += (temp - somComp[i][0]) * alpha;
        somComp[i][1] += (comp - somComp[i][1]) * alpha;
    }
}

unsigned statusSom(char *buffer) {
    char *end = buffer;

    for(int i = 0; i < SOM_NODE_CNT; i++) {
        end += fmtFloat(somComp[i][0], 9, 4, end);
        end += fmtFloat(somComp[i][1] * 1e6f, 12, 4, end);
        *(end++) = '\n';
    }

    return end - buffer;
}
