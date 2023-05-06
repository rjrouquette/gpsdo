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
#define TEMP_SLOW_CNT (3)

#define TCMP_SAVE_INTV (3600) // save state every hour
#define TCMP_UPDT_INTV (CLK_FREQ / 16)  // 16 Hz

#define SOM_EEPROM_BASE (0x0020)
#define SOM_NODE_CNT (32)
#define SOM_FILL_OFF (2.0f)
#define SOM_FILL_REG (8.0f)

#define REG_MIN_RMSE (100e-9f)

static volatile uint32_t tempNext = 0;
static volatile float tempValue;

static volatile uint32_t tcmpNext = 0;
static volatile uint32_t tcmpSaved;
static volatile float tcmpValue;

static volatile float tcmpMean[2];
static volatile float tcmpCoeff[3];
static volatile float tcmpRmse;

// SOM for filtering compensation samples
static volatile float somComp[SOM_NODE_CNT][3];
static volatile float somFill;

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
        1.18506486e-27f
};

__attribute__((always_inline))
static inline float toCelsius(int32_t adcValue) {
    return -0.0604248047f * (float) (adcValue - 2441);
}

static void loadSom();
static void saveSom();
static void seedSom(float temp, float comp);
static void updateSom(float temp, float comp);
static void updateRegression();

/**
 * Estimate temperature correction using 3rd order taylor series
 * @param temp current temperature in Celsius
 * @return estimated correction value
 */
static float tcmpEstimate(float temp);


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

    // take and discard some initial measurements
    for(int i = 0; i < 16; i++) {
        ADC0.PSSI.SS3 = 1;      // trigger temperature measurement
        while(!ADC0.RIS.INR3);  // wait for data
        ADC0.ISC.IN3 = 1;       // clear flag
        // drain FIFO
        while(!ADC0.SS3.FSTAT.EMPTY)
            tempValue = toCelsius(ADC0.SS3.FIFO.DATA);
    }

    loadSom();
    if(isfinite(somComp[0][0])) {
        updateRegression();
        tcmpValue = tcmpEstimate(tempValue);
    }
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
        // update temperature compensation
        tcmpValue = tcmpEstimate(tempValue);
    }
}

float TCMP_temp() {
    return tempValue;
}

float TCMP_get() {
    return tcmpValue;
}

void TCMP_update(const float target) {
    updateSom(tempValue, target);
    updateRegression();

    tcmpValue = tcmpEstimate(tempValue);

    const uint32_t now = CLK_MONO_INT();
    if(now - tcmpSaved > TCMP_SAVE_INTV) {
        tcmpSaved = now;
        saveSom();
    }
}

unsigned TCMP_status(char *buffer) {
    char tmp[32];
    char *end = buffer;

    end = append(end, "tcomp status:\n");

    tmp[fmtFloat(tempValue, 12, 4, tmp)] = 0;
    end = append(end, "  - temp:    ");
    end = append(end, tmp);
    end = append(end, " C\n");

    tmp[fmtFloat(somFill, 12, 4, tmp)] = 0;
    end = append(end, "  - fill:    ");
    end = append(end, tmp);
    end = append(end, "\n");

    tmp[fmtFloat(tcmpRmse * 1e6f, 12, 4, tmp)] = 0;
    end = append(end, "  - rmse:    ");
    end = append(end, tmp);
    end = append(end, " ppm\n");

    tmp[fmtFloat(tcmpMean[0], 12, 4, tmp)] = 0;
    end = append(end, "  - mean[0]: ");
    end = append(end, tmp);
    end = append(end, " C\n");

    tmp[fmtFloat(tcmpMean[1] * 1e6f, 12, 4, tmp)] = 0;
    end = append(end, "  - mean[1]: ");
    end = append(end, tmp);
    end = append(end, " ppm\n");

    tmp[fmtFloat(tcmpCoeff[0] * 1e6f, 12, 4, tmp)] = 0;
    end = append(end, "  - coef[0]: ");
    end = append(end, tmp);
    end = append(end, " ppm/C\n");

    tmp[fmtFloat(tcmpCoeff[1] * 1e6f, 12, 4, tmp)] = 0;
    end = append(end, "  - coef[1]: ");
    end = append(end, tmp);
    end = append(end, " ppm/C^2\n");

    tmp[fmtFloat(tcmpCoeff[2] * 1e6f, 12, 4, tmp)] = 0;
    end = append(end, "  - coef[2]: ");
    end = append(end, tmp);
    end = append(end, " ppm/C^3\n");

    tmp[fmtFloat(tcmpValue * 1e6f, 12, 4, tmp)] = 0;
    end = append(end, "  - curr:    ");
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
    float norm = 0.01f / (float) SOM_NODE_CNT;
    int mid = SOM_NODE_CNT / 2;
    for(int i = 0; i < SOM_NODE_CNT; i++) {
        somComp[i][0] = temp + (norm * (float)(i - mid));
        somComp[i][1] = comp;
        somComp[i][2] = 0;
    }
}

static void updateSom(float temp, float comp) {
    // initialize som nodes if necessary
    if(!isfinite(somComp[0][0])) {
        seedSom(temp, comp);
    }

    // locate nearest node and measure learning progress
    float alpha = 0;
    int best = 0;
    float dist = fabsf(temp - somComp[0][0]);
    for(int i = 1; i < SOM_NODE_CNT; i++) {
        alpha += somComp[i][2];
        float diff = fabsf(temp - somComp[i][0]);
        if(diff < dist) {
            dist = diff;
            best = i;
        }
    }

    // update nodes using dynamic learning rate
    somFill = alpha;
    alpha = 1.0f / (1.0f + (alpha * alpha));
    for(int i = 0; i < SOM_NODE_CNT; i++) {
        // compute neighbor distance
        int ndist = i - best;
        if(ndist < 0) ndist = -ndist;
        // compute node alpha
        const float w = alpha * somNW[ndist];
        // update node weights
        somComp[i][0] += (temp - somComp[i][0]) * w;
        somComp[i][1] += (comp - somComp[i][1]) * w;
        somComp[i][2] += (1.0f - somComp[i][2]) * w;
    }
}

static void updateRegression() {
    if(somFill < SOM_FILL_OFF)
        return;

    // compute means
    float mean[3] = {0, 0, 0};
    for(int i = 0; i < SOM_NODE_CNT; i++) {
        const volatile float *row = somComp[i];
        mean[0] += row[0] * row[2];
        mean[1] += row[1] * row[2];
        mean[2] += row[2];
    }
    mean[0] /= mean[2];
    mean[1] /= mean[2];
    // update means
    tcmpMean[0] = mean[0];
    tcmpMean[1] = mean[1];

    if(somFill < SOM_FILL_REG)
        return;

    // scratch variables
    float coeff[3];
    float scratch[32][2];
    float xx, xy;

    // linear coefficient
    xx = 0; xy = 0;
    for(int i = 0; i < SOM_NODE_CNT; i++) {
        const volatile float *row = somComp[i];
        float x = scratch[i][0] = row[0] - mean[0];
        float y = scratch[i][1] = row[1] - mean[1];
        xx += x * x * row[2];
        xy += x * y * row[2];
    }
    coeff[0] = xy / xx;

    // quadratic coefficient
    xx = 0; xy = 0;
    for(int i = 0; i < SOM_NODE_CNT; i++) {
        const volatile float *row = somComp[i];
        float x = scratch[i][0];
        float y = (scratch[i][1] -= x * coeff[0]);
        x = x * x;
        xx += x * x * row[2];
        xy += x * y * row[2];
    }
    coeff[1] = xy / xx;

    // cubic coefficient
    xx = 0; xy = 0;
    for(int i = 0; i < SOM_NODE_CNT; i++) {
        const volatile float *row = somComp[i];
        float x = scratch[i][0];
        float y = (scratch[i][1] -= x * x * coeff[1]);
        x = x * x * x;
        xx += x * x * row[2];
        xy += x * y * row[2];
    }
    coeff[2] = xy / xx;


    // compute RMS error
    float rmse = 0;
    for(int i = 0; i < SOM_NODE_CNT; i++) {
        const volatile float *row = somComp[i];

        float x = row[0] - mean[0];
        float y = mean[1];
        y += x * coeff[0];
        y += x * x * coeff[1];
        y += x * x * x * coeff[2];

        float error = y - row[1];
        rmse += error * error * row[2];
    }
    tcmpRmse = sqrtf(rmse / mean[2]);

    // update coefficients
    if(tcmpRmse <= REG_MIN_RMSE) {
        tcmpCoeff[0] = coeff[0];
        tcmpCoeff[1] = coeff[1];
        tcmpCoeff[2] = coeff[2];
    }
}

static float tcmpEstimate(const float temp) {
    float x = temp - tcmpMean[0];
    float y = tcmpMean[1];
    y += x * tcmpCoeff[0];
    y += x * x * tcmpCoeff[1];
    y += x * x * x * tcmpCoeff[2];
    return y;
}

unsigned statusSom(char *buffer) {
    char *end = buffer;

    // export in C-style array syntax
    end = append(end, "float som[32][3] = {\n");
    for(int i = 0; i < SOM_NODE_CNT; i++) {
        end += fmtFloat(somComp[i][0], 0, -1, end);
        end = append(end, ", ");
        end += fmtFloat(somComp[i][1] * 1e6f, 0, -1, end);
        end = append(end, ", ");
        end += fmtFloat(somComp[i][2], 0, -1, end);
        if(i < SOM_NODE_CNT - 1)
            *(end++) = ',';
        *(end++) = '\n';
    }
    end = append(end, "};\n");

    return end - buffer;
}
