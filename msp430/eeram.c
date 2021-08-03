
#include "eeram.h"
#include "i2c.h"

// magic number to indicate initialized EERAM
#define EERAM_MAGIC (0xef58b6d0fc8e1257ull)
// EERAM is organinzed into 8 KiB segments
#define EERAM_SIZE (8192u)

/**
 * Init EERAM
**/
void EERAM_init() {
    // Verify that EERAM has been initialized
    uint64_t magic;
    EERAM_read(EERAM_CSA0, 0, &magic, sizeof(uint64_t));
    if(magic == EERAM_MAGIC) return;
    // initialize EERAM
    EERAM_reset();
}

/**
 * Reset EERAM
**/
void EERAM_reset() {
    // clear all EERAM bytes
    EERAM_bzero(EERAM_CSA0, 0, EERAM_SIZE);
    EERAM_bzero(EERAM_CSA1, 0, EERAM_SIZE);
    // write magic number
    uint64_t magic = EERAM_MAGIC;
    EERAM_write(EERAM_CSA0, 0, &magic, sizeof(uint64_t));
}

/**
 * Read EERAM block
**/
void EERAM_read(uint8_t csa, uint16_t addr, void *data, uint16_t size) {
    I2C_startWrite(csa);
    I2C_write(&addr, 2);
    I2C_startRead(csa);
    I2C_read(data, size);
    I2C_stop();
}

/**
 * Write EERAM block
**/
void EERAM_write(uint8_t csa, uint16_t addr, const void *data, uint16_t size) {
    I2C_startWrite(csa);
    I2C_write(&addr, 2);
    I2C_write(data, size);
    I2C_stop();
}

/**
 * Clear EERAM block
**/
void EERAM_bzero(uint8_t csa, uint16_t addr, uint16_t size) {
    I2C_startWrite(csa);
    I2C_write(&addr, 2);

    // prepare zeroed word
    uint64_t zero = 0ull;
    // write non-word bytes
    I2C_write(&zero, size & 7u);
    // reduce remaining count to words
    size = size >> 3u;
    // write remaining words
    while(size-- > 0)
        I2C_write(&zero, 8u);
    // end I2C transaction
    I2C_stop();
}
