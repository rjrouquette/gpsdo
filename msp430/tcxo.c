
#include "math.h"
#include "tcxo.h"

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
    int64_t rem = currBin.mat[cidx];
    int32_t quo;
    divS(currBin.norm, &rem, &quo);
    return quo;
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
    uint8_t binId = getBinId(tempC);
    loadBin(binId);

    int32_t A = getCell(0); // 0.32
    int32_t B = getCell(1); // 16.16
    int32_t C = getCell(2); // 24.8
    int32_t D = getCell(3); // 32.0

    int32_t Z = A - mult32s(B, B);
    // require stable data
    if(Z <= 0) return D;
    // require at least 64 samples
    if(currBin.norm < 0x200000u) return D;

    int32_t m, b;
    int64_t rem;
    rem = (mult64s(C, 256) - mult64s(B, D)) << 24;
    divS(Z, &rem, &m);
    rem = mult64s(A, D) - (mult64s(B, C) << 8);
    divS(Z, &rem, &b);

    int16_t x = getFracTemp(tempC);
    return (mult64s(m, x) >> 16u) + b;
}

/**
 * Update the TCXO internal state with the given sample.
 * @param tempC - temperature in Celsius and 8.8 fixed point format
 * @param offset - the DCXO digital frequency offset in 32.0 fixed point format
**/
void TCXO_updateCompensation(int16_t tempC, int32_t offset) {
    // compute binId
    uint8_t binId = getBinId(tempC);
    loadBin(binId);

    int16_t x = getFracTemp(tempC);
    increment();
    accumulate(0, mult32s(x, x) << 16u);
    accumulate(1, x << 8u);
    accumulate(2, mult64s(offset, x));
    accumulate(3, offset);
    storeBin();
}
