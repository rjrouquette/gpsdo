
#include "math.h"
#include "tcxo.h"

#define XX (0)
#define X1 (1)
#define YX (2)
#define Y1 (3)

static uint8_t currBinIdx = 0;

static struct TempBin {
    int64_t mat[4];
    uint32_t norm;
} currBin;

// extract bin ID from temperatute
static uint8_t getBinId(int16_t tempC) {
    uint8_t offset = tempC >> 8u;
    return offset + 64u;
}

// reduce to fractional temperature (0.16 fixed point format)
static int8_t getFracTemp(int16_t tempC) {
    int8_t x = (int8_t) (tempC & ((int16_t) 0xff));
    x -= (int8_t) 128;
    return x;
}

// A = A + ((B - A) / 2^16)
static void increment() {
    currBin.norm -= currBin.norm >> 16u;
    currBin.norm += 1u << 15u;
}

// A = A + ((B - A) / 2^16)
static void accumulate(uint8_t i, const int64_t value) {
    currBin.mat[i] -= currBin.mat[i] >> 16u;
    currBin.mat[i] += value << 15u;
}

// get normalized cell contents
static int32_t getCell(uint8_t cidx) {
    return div64s32u(currBin.mat[cidx], currBin.norm);
}

// load the specified temperature bin into memory
static void loadBin(uint8_t binId) {
    if(binId == currBinIdx) return;

    // TODO load bin data from I2C EERAM
    currBinIdx = binId;
}

// load the specified temperature bin into memory
static void storeBin() {
    uint16_t addr = currBinIdx;
    addr *= sizeof(struct TempBin);

    
    // TODO store bin data to I2C EERAM
}

/**
 * Gets the DCXO adjustment for the given temperature.
 * @param tempC - temperature in Celsius and 8.8 fixed point format
 * @return the DCXO digital frequency offset
**/
int32_t TCXO_getCompensation(int16_t tempC) {
    loadBin(getBinId(tempC));

    // output -2^31 if there is no data
    if(currBin.norm == 0) return 0x80000000l;

    // load Y1 cell (average offset)
    const int32_t D = getCell(Y1); // 32.0
    // require at least 64 samples before performing OLS fit
    if(currBin.norm < 0x200000u) return D;

    // load XX cells
    const int32_t A = getCell(XX); // 0.32
    const int32_t B = getCell(X1); // 16.16
    // compute determinant
    const int32_t Z = A - mult32s(B, B);
    // must satisfy E(x^2) > E(x)^2
    if(Z <= 0) return D;

    // load YX cell
    const int32_t C = getCell(YX); // 24.8
    // C is not left shifted here because HW multipier only supports 32-bit operands

    // compute m
    const int32_t m = div64s32u(
        // C is multiplied by 256 to correct alignment (equivalent to left shift of 8)
        (mult64s(C, 256) - mult64s(B, D)) << 24u,
        // divide by determinant of XX
        Z
    );

    // compute b
    const int32_t b = div64s32u(
        // B * C is left shifted by 8 to correct alignment
        mult64s(A, D) - (mult64s(B, C) << 8u),
        // divide by determinant of XX
        Z
    );

    // get fractional part of temperature
    const int16_t x = getFracTemp(tempC);
    // apply coefficients
    return (mult64s(m, x) >> 16u) + b;
}

/**
 * Update the TCXO internal state with the given sample.
 * @param tempC - temperature in Celsius and 8.8 fixed point format
 * @param offset - the DCXO digital frequency offset in 32.0 fixed point format
**/
void TCXO_updateCompensation(int16_t tempC, int32_t offset) {
    // load temperature bin
    loadBin(getBinId(tempC));

    // get fractional part of temperature
    int16_t x = getFracTemp(tempC);
    // update accumulators
    increment();
    accumulate(XX, mult32s(x, x) << 16u);
    accumulate(X1, x << 8u);
    accumulate(YX, mult64s(offset, x));
    accumulate(Y1, offset);
    // store results
    storeBin();
}
