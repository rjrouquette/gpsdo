//
// Created by robert on 5/12/22.
//

#ifndef GPSDO_CRC_H
#define GPSDO_CRC_H

#include "register.h"

enum CRC_TYPE {
    CRC_TYPE_8005 = 0x0,
    CRC_TYPE_1021 = 0x1,
    CRC_TYPE_04C11DB7 = 0x2,
    CRC_TYPE_1EDC6F41 = 0x3,

    CRC_TYPE_TCP = 0x8,
};

enum CRC_ENDIAN {
    CRC_ENDIAN_LITTLE = 0x0,
    CRC_ENDIAN_HALF = 0x1,
    CRC_ENDIAN_WORD = 0x2,
    CRC_ENDIAN_BIG = 0x3
};

enum CRC_INIT {
    CRC_INIT_SEED = 0x0,

    CRC_INIT_ZERO = 0x2,
    CRC_INIT_FULL = 0x3
};

PAGE_MAP (CRC_MAP, {
    char reserved_0[0x400];

    // offset 0x400
    REGMAP_32(, {
        enum CRC_TYPE TYPE: 4;
        enum CRC_ENDIAN ENDIAN: 2;
        unsigned : 1;
        unsigned BR: 1;
        unsigned OBR: 1;
        unsigned RESINV: 1;
        unsigned : 2;
        unsigned SIZE: 1;
        enum CRC_INIT INIT: 2;
    }) CTRL;

    char reserved_1[0x00C];

    // offset 0x410
    uint32_t SEED;
    // offset 0x414
    uint32_t DIN;
    // offset 0x418
    uint32_t RSLTPP;

    char reserved_2[0xBE4];
})

#define CRC (*(volatile struct CRC_MAP *)0x44030000)

#endif //GPSDO_CRC_H
