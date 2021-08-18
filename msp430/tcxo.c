
#include "eeram.h"
#include "math.h"
#include "tcxo.h"

// start of temperature bins in EERAM
#define EERAM_OFFSET (0x0500u)
// temperature limits
#define TEMP_MIN (-64 << 8u)  // -64 Celsius
#define TEMP_MAX (127 << 8u)  // 127 Celsius

// cell indices
#define XX (0)
#define X1 (1)
#define YX (2)
#define Y1 (3)

// temperate bin
struct TempBin {
    int64_t mat[4];
    uint32_t norm;
};

// internal state
static struct TempBin currBin;
static uint16_t currBinIdx = -1u;

// reduce to fractional temperature (0.16 fixed point format)
static int16_t getFracTemp(int16_t tempC) {
    return (tempC & 0xFF) - 128;
}

// A = A + ((B - A) / 2^16)
static void increment() {
    currBin.norm -= currBin.norm >> 16u;
    currBin.norm += 1u << 15u;
}

// A = A + ((B - A) / 2^16)
static void accumulate(uint8_t i, const int32_t value) {
    currBin.mat[i] -= currBin.mat[i] >> 16u;
    currBin.mat[i] += ((int64_t)value) << 15u;
}

// get normalized cell contents
static int32_t getCell(uint8_t cidx) {
    return div64s32u(currBin.mat[cidx], currBin.norm);
}

// load the specified temperature bin into memory
static uint16_t loadBin(int16_t tempC) {
    // check for invalid temperatures
    if(tempC < TEMP_MIN) return 1;
    if(tempC > TEMP_MAX) return 1;

    // extract bin ID from temperature
    uint16_t binId = (tempC >> 8u) & 0xFFu;
    // bin 0 = -64 Celsius
    binId += 64u;

    // skip EERAM load if binID has not changed
    if(binId == currBinIdx)
        return 0;

    // load bin data from EERAM
    currBinIdx = binId;
    binId *= sizeof(struct TempBin);
    binId += EERAM_OFFSET;
    EERAM_read(binId, &currBin, sizeof(struct TempBin));
    return 0;
}

/**
 * Gets the DCXO adjustment for the given temperature.
 * @param tempC - temperature in Celsius and 8.8 fixed point format
 * @return the DCXO digital frequency offset
**/
int32_t TCXO_getCompensation(int16_t tempC) {
    // load temperature bin
    if(loadBin(tempC)) return TCXO_ERR;

    // return error if there is no data
    if(currBin.norm == 0) return TCXO_ERR;

    // load Y1 cell (average offset)
    const int32_t D = getCell(Y1); // 24.8
    // require at least 64 samples before performing OLS fit
    if(currBin.norm < 0x200000u) return D >> 8;

    // load XX cells
    const int32_t A = getCell(XX); // 0.32
    union {
        int16_t word[2];
        int32_t full;
    } B;
    B.word[1] = getCell(X1); // 0.16
    B.word[0] = 0;
    // compute determinant (0.32)
    const int32_t Z = A - mult16s16s(B.word[1], B.word[1]);
    // must satisfy E(x^2) > E(x)^2
    if(Z <= 0) return D >> 8;

    // load YX cell
    union {
        int32_t word[2];
        int64_t full;
    } C;
    C.word[1] = getCell(YX); // 24.8;
    C.word[0] = 0;

    // compute m (24.8)
    const int32_t m = div64s32u(
        C.full - mult32s32s(B.full, D),
        Z
    );

    // compute b (24.8)
    const int32_t b = div64s32u(
        mult32s32s(A, D) - mult32s32s(B.full, C.word[1]),
        Z
    );

    // get fractional part of temperature
    const int16_t x = getFracTemp(tempC);
    // apply coefficients
    return (mult32s32s(m, x) + mult32s32s(b, 256)) >> 16;
}

/**
 * Update the TCXO internal state with the given sample.
 * @param tempC - temperature in Celsius and 8.8 fixed point format
 * @param offset - the DCXO digital frequency offset in 32.0 fixed point format
**/
void TCXO_updateCompensation(int16_t tempC, int32_t offset) {
    if(loadBin(tempC)) return;

    // get fractional part of temperature
    const int16_t x = getFracTemp(tempC) << 8;
    // update accumulators
    increment();
    accumulate(XX, mult16s16s(x, x));
    accumulate(X1, x);
    accumulate(YX, mult24s8s(offset, x));
    accumulate(Y1, offset << 8);

    // save results to EERAM
    uint16_t addr = currBinIdx;
    addr *= sizeof(struct TempBin);
    addr += EERAM_OFFSET;
    EERAM_write(addr, &currBin, sizeof(struct TempBin));
}
