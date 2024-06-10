//
// Created by robert on 4/30/23.
//

#include "tcmp.hpp"

#include "../delay.hpp"
#include "../format.hpp"
#include "../run.hpp"
#include "../../hw/adc.h"
#include "../../hw/eeprom.h"
#include "../clock/capture.hpp"
#include "../clock/comp.hpp"
#include "../clock/mono.hpp"

#include <cmath>

#define INTV_REGR (1u << (24 - 14)) // 16384 Hz

#define TCMP_SAVE_INTV (3600) // save state every hour

#define SOM_EEPROM_BASE (0x0020)
#define SOM_NODE_DIM (3)
#define SOM_NODE_CNT (16)
#define SOM_FILL_OFF (0.2f)
#define SOM_FILL_REG (4.0f)
#define SOM_RATE (0x1p-12f)

#define REG_DIM_COEF (3)
#define REG_MIN_RMSE (250e-9f)


// sample ring size
static constexpr int RING_SIZE = 256;
// sample ring mask
static constexpr int RING_MASK = RING_SIZE - 1;
// ring position
static int ringPos = 0;
// ring buffer
static float tempRing[RING_SIZE];
static volatile float tempValue;
static volatile float tempNoise;
static int initCounter;

static volatile uint32_t tcmpSaved;
static volatile float tcmpValue;

static volatile float tcmpMean[SOM_NODE_DIM];
static volatile float tcmpCoeff[REG_DIM_COEF];
static volatile float tcmpRmse;

// SOM filter for compensation samples
static float somNode[SOM_NODE_CNT][SOM_NODE_DIM];
static float somResidual[SOM_NODE_CNT][SOM_NODE_DIM];
static float somNW[SOM_NODE_CNT];

// regression step counter
static uint32_t regressionStep = 0;

static void loadSom();
static void saveSom();
static void seedSom(float temp, float comp);
static void updateSom(float temp, float comp, float alpha);
static void runRegression(void *ref);

static void computeMean(const float *data);
static void fitQuadratic(const float *data);
static void computeMSE(const float *data);

/**
 * Estimate temperature correction using quadratic regressor
 * @param temp current temperature in Celsius
 * @return estimated correction value
 */
static float tcmpEstimate(float temp);

// update mean and standard deviation
static void runTemperature([[maybe_unused]] void *ref) {
    // update temperature measurement
    const auto temp = clock::capture::temperature();
    const auto head = (ringPos + 1) & RING_MASK;
    tempRing[head] = temp;
    ringPos = head;
}

/**
 * Update the current temperature compensation value.
 * @param ref unused context argument
 */
static void runCompensation(void *ref) {
    // wait for ring to fill with valid data
    if (initCounter < 68) {
        ++initCounter;
        return;
    }

    // constant terms
    static constexpr auto cc = static_cast<float>(RING_SIZE);
    static constexpr auto xc_ = 0.5f * cc * (cc - 1.0f);
    static constexpr auto xx_ = 2.0f * (cc - 0.5f) * xc_ / 3.0f;
    static constexpr auto xc = xc_ / cc;
    static constexpr auto xx = xx_ / cc;

    // dynamic terms
    float yc = 0, yx = 0;
    const auto head = ringPos;
    for (int i = 0; i < RING_SIZE; ++i) {
        const auto x = static_cast<float>(i);
        const auto y = tempRing[(head - i) & RING_MASK];
        yc += y;
        yx += y * x;
    }
    yc /= cc;
    yx /= cc;

    // compute offset and slope
    const auto slope = (yx - yc * xc) / (xx - xc * xc);
    const auto offset = yc - xc * slope;

    // compute mean-squared error
    float mse = 0;
    for (int i = 0; i < RING_SIZE; ++i) {
        const auto x = static_cast<float>(i);
        const auto y = tempRing[(head - i) & RING_MASK];
        const float error = y - offset - x * slope;
        mse += error * error;
    }
    mse /= cc;

    // compute variance
    const auto slopeVariance = mse / (xx - xc * xc);
    const auto offsetVariance = slopeVariance * xx;

    // update temperature compensation
    const auto trim = static_cast<int32_t>(0x1p32f * tcmpEstimate(offset));
    clock::compensated::setTrim(trim);
    // update global variables
    tcmpValue = 0x1p-32f * static_cast<float>(trim);
    tempValue = offset;
    tempNoise = std::sqrt(offsetVariance);
}

void tcmp::init() {
    loadSom();
    if (std::isfinite(somNode[0][0]))
        runSleep(INTV_REGR, runRegression, nullptr);

    // schedule tasks
    runPeriodic(RUN_SEC / 16, runTemperature, nullptr);
    runPeriodic(RUN_SEC / 4, runCompensation, nullptr);
}

float tcmp::temp() {
    return tempValue;
}

float tcmp::get() {
    return tcmpValue;
}

void tcmp::update(const float target, const float weight) {
    updateSom(tempValue, target, weight);

    const uint32_t now = clock::monotonic::seconds();
    if (now - tcmpSaved > TCMP_SAVE_INTV) {
        tcmpSaved = now;
        saveSom();
    }

    if (regressionStep == 0) {
        // start regression computation
        runSleep(INTV_REGR, runRegression, nullptr);
    }
    else {
        // re-start regression computation
        regressionStep = 0;
    }
}

unsigned tcmp::status(char *buffer) {
    char tmp[32];
    char *end = buffer;

    end = append(end, "tcomp status:\n");

    tmp[fmtFloat(tempValue, 12, 4, tmp)] = 0;
    end = append(end, "  - temp:    ");
    end = append(end, tmp);
    end = append(end, " C\n");

    tmp[fmtFloat(tempNoise, 12, 4, tmp)] = 0;
    end = append(end, "  - noise:   ");
    end = append(end, tmp);
    end = append(end, " C\n");

    tmp[fmtFloat(tcmpMean[2], 12, 4, tmp)] = 0;
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
    end = append(end, " ppm\n");

    tmp[fmtFloat(tcmpCoeff[1] * 1e6f, 12, 4, tmp)] = 0;
    end = append(end, "  - coef[1]: ");
    end = append(end, tmp);
    end = append(end, " ppm/C\n");

    tmp[fmtFloat(tcmpCoeff[2] * 1e6f, 12, 4, tmp)] = 0;
    end = append(end, "  - coef[2]: ");
    end = append(end, tmp);
    end = append(end, " ppm/C^2\n");

    tmp[fmtFloat(tcmpValue * 1e6f, 12, 4, tmp)] = 0;
    end = append(end, "  - curr:    ");
    end = append(end, tmp);
    end = append(end, " ppm\n\n");

    return end - buffer;
}

static void loadSom() {
    // initialize neighbor weights
    for (int i = 0; i < SOM_NODE_CNT; i++)
        somNW[i] = std::exp(-2.0f * static_cast<float>(i * i));

    auto ptr = reinterpret_cast<uint32_t*>(somNode);
    const auto end = ptr + (sizeof(somNode) / sizeof(uint32_t));

    // load SOM data
    EEPROM_seek(SOM_EEPROM_BASE);
    while (ptr < end)
        *ptr++ = EEPROM_read();
}

static void saveSom() {
    auto ptr = reinterpret_cast<uint32_t*>(somNode);
    const auto end = ptr + (sizeof(somNode) / sizeof(uint32_t));

    // load SOM data
    EEPROM_seek(SOM_EEPROM_BASE);
    while (ptr < end)
        EEPROM_write(*(ptr++));
}

static void seedSom(const float temp, const float comp) {
    constexpr int mid = SOM_NODE_CNT / 2;
    for (int i = 0; i < SOM_NODE_CNT; i++) {
        somNode[i][0] = temp + 0.1f * static_cast<float>(i - mid);
        somNode[i][1] = comp;
        somNode[i][2] = 0;
    }
}

inline void accumulate(float &accumulator, float &residual, const float delta) {
    const auto initial = accumulator;
    accumulator += residual + delta;
    residual += delta - (accumulator - initial);
}

static void updateSom(const float temp, const float comp, float alpha) {
    // initialize som nodes if necessary
    if (!std::isfinite(somNode[0][0])) {
        if (alpha >= 0.5f)
            seedSom(temp, comp);
        return;
    }

    // locate nearest node
    int best = 0;
    float dist = std::abs(temp - somNode[0][0]);
    for (int i = 1; i < SOM_NODE_CNT; i++) {
        const float diff = std::abs(temp - somNode[i][0]);
        if (diff < dist) {
            dist = diff;
            best = i;
        }
    }

    // update nodes using dynamic learning rate
    alpha *= SOM_RATE;
    for (int i = 0; i < SOM_NODE_CNT; i++) {
        // compute node alpha
        const float w = alpha * somNW[std::abs(i - best)];
        // update node weights
        accumulate(somNode[i][0], somResidual[i][0], (temp - somNode[i][0]) * w);
        accumulate(somNode[i][1], somResidual[i][1], (comp - somNode[i][1]) * w);
        accumulate(somNode[i][2], somResidual[i][2], (1.0f - somNode[i][2]) * w);
    }
}

// regression scratch variables
static float mse;
static float mean[SOM_NODE_DIM];
static float coef[REG_DIM_COEF];
float scratch[SOM_NODE_CNT][SOM_NODE_DIM];

static int step0() {
    // compute initial mean
    computeMean(somNode[0]);
    return 0;
}

static int step1() {
    // require sufficient data
    if (mean[2] < SOM_FILL_OFF)
        return 1;

    // require sufficient data
    if (mean[2] < SOM_FILL_REG) {
        // only update means
        tcmpMean[0] = mean[0];
        tcmpMean[1] = mean[1];
        tcmpMean[2] = mean[2];
        return 2;
    }

    return 0;
}

static int step2() {
    // compute initial fit
    fitQuadratic(somNode[0]);
    return 0;
}

static int step3() {
    // compute residual error
    computeMSE(somNode[0]);
    return 0;
}

static int step4() {
    // reweigh outliers
    for (int i = 0; i < SOM_NODE_CNT; i++) {
        const float x = (scratch[i][0] = somNode[i][0]) - mean[0];
        float y = (scratch[i][1] = somNode[i][1]) - mean[1];
        y -= coef[0];
        y -= x * coef[1];
        y -= x * x * coef[2];
        scratch[i][2] = somNode[i][2] * std::exp(-0.25f * y * y / mse);
    }
    return 0;
}

static int step5() {
    // compute final mean
    computeMean(scratch[0]);
    return 0;
}

static int step6() {
    // compute final fit
    fitQuadratic(scratch[0]);
    return 0;
}

static int step7() {
    // compute residual error
    computeMSE(scratch[0]);
    return 0;
}

static int step8() {
    // update residual error
    tcmpRmse = std::sqrt(mse);

    // quality check fit
    if (tcmpRmse <= REG_MIN_RMSE) {
        // update means
        tcmpMean[0] = mean[0];
        tcmpMean[1] = mean[1];
        tcmpMean[2] = mean[2];
        // update coefficients
        tcmpCoeff[0] = coef[0];
        tcmpCoeff[1] = coef[1];
        tcmpCoeff[2] = coef[2];
    }
    return 0;
}

#define STEP_CNT (9)
typedef int (*RegressionStep)();
static constexpr RegressionStep steps[STEP_CNT] = {
    step0,
    step1,
    step2,
    step3,
    step4,
    step5,
    step6,
    step7,
    step8
};

static void runRegression(void *ref) {
    if (
        regressionStep >= STEP_CNT ||
        (*(steps[regressionStep]))() != 0
    ) {
        runCancel(runRegression, nullptr);
        regressionStep = 0;
    }
    else {
        ++regressionStep;
    }
}

static float tcmpEstimate(const float temp) {
    const float x = temp - tcmpMean[0];
    float y = tcmpCoeff[0];
    y += x * tcmpCoeff[1];
    y += x * x * tcmpCoeff[2];
    return y + tcmpMean[1];
}

unsigned statusSom(char *buffer) {
    char *end = buffer;

    // export in C-style array syntax
    end = append(end, "float som[16][3] = {\n");
    for (int i = 0; i < SOM_NODE_CNT; i++) {
        end += fmtFloat(somNode[i][0], 0, -1, end);
        end = append(end, ", ");
        end += fmtFloat(somNode[i][1] * 1e6f, 0, -1, end);
        end = append(end, ", ");
        end += fmtFloat(somNode[i][2], 0, -1, end);
        if (i < SOM_NODE_CNT - 1)
            *end++ = ',';
        *end++ = '\n';
    }
    end = append(end, "};\n");

    return end - buffer;
}

static void computeMean(const float *data) {
    // compute means
    mean[0] = 0;
    mean[1] = 0;
    mean[2] = 0;
    for (int i = 0; i < SOM_NODE_CNT; i++) {
        mean[0] += data[0] * data[2];
        mean[1] += data[1] * data[2];
        mean[2] += data[2];
        data += SOM_NODE_DIM;
    }
    mean[0] /= mean[2];
    mean[1] /= mean[2];
}

static void fitQuadratic(const float *data) {
    // compute equation matrix
    float xx[REG_DIM_COEF][REG_DIM_COEF + 1] = {0};
    for (int i = 0; i < SOM_NODE_CNT; i++) {
        const float x = data[0] - mean[0];
        const float y = data[1] - mean[1];
        float z = data[2];
        data += SOM_NODE_DIM;

        // constant terms
        xx[2][2] += z;
        xx[2][3] += z * y;

        // linear terms
        z *= x;
        xx[1][2] += z;
        xx[1][3] += z * y;

        // quadratic terms
        z *= x;
        xx[0][2] += z;
        xx[0][3] += z * y;

        // cubic terms
        z *= x;
        xx[0][1] += z;

        // quartic terms
        z *= x;
        xx[0][0] += z;
    }
    // fill remaining cells using matrix symmetry
    xx[1][0] = xx[0][1];
    xx[1][1] = xx[0][2];
    xx[2][0] = xx[0][2];
    xx[2][1] = xx[1][2];

    // row-echelon reduction
    {
        const float f = xx[1][0] / xx[0][0];
        xx[1][1] -= f * xx[0][1];
        xx[1][2] -= f * xx[0][2];
        xx[1][3] -= f * xx[0][3];
    }

    // row-echelon reduction
    {
        const float f = xx[2][0] / xx[0][0];
        xx[2][1] -= f * xx[0][1];
        xx[2][2] -= f * xx[0][2];
        xx[2][3] -= f * xx[0][3];
    }

    // row-echelon reduction
    {
        const float f = xx[2][1] / xx[1][1];
        xx[2][2] -= f * xx[1][2];
        xx[2][3] -= f * xx[1][3];
    }

    // compute coefficients
    coef[0] = xx[2][3] / xx[2][2];
    coef[1] = (xx[1][3] - xx[1][2] * coef[0]) / xx[1][1];
    coef[2] = (xx[0][3] - xx[0][2] * coef[0] - xx[0][1] * coef[1]) / xx[0][0];
}

static void computeMSE(const float *data) {
    float acc = 0;
    for (int i = 0; i < SOM_NODE_CNT; i++) {
        float x = data[0] - mean[0];
        float y = data[1] - mean[1];
        y -= coef[0];
        y -= x * coef[1];
        y -= x * x * coef[2];
        acc += y * y * data[2];
        data += SOM_NODE_DIM;
    }
    mse = acc / mean[2];
}
