/**
 * Synchronous Serial Interfaces
 * @author Robert J. Rouquette
 * @date 2022-04-15
 */

#ifndef TM4C_SSI_H
#define TM4C_SSI_H

#include "register.h"
#include "gpio.h"

enum SSI_CLK_SRC_e {
    SSI_CLK_SYS = 0,
    SSI_CLK_ALT = 5
};

enum SSI_DSS_e {
    SSI_DSS_4_BIT = 3,
    SSI_DSS_5_BIT = 4,
    SSI_DSS_6_BIT = 5,
    SSI_DSS_7_BIT = 6,
    SSI_DSS_8_BIT = 7,
    SSI_DSS_9_BIT = 8,
    SSI_DSS_10_BIT = 9,
    SSI_DSS_11_BIT = 10,
    SSI_DSS_12_BIT = 11,
    SSI_DSS_13_BIT = 12,
    SSI_DSS_14_BIT = 13,
    SSI_DSS_15_BIT = 14,
    SSI_DSS_16_BIT = 15,
};

PAGE_MAP (QSSI_MAP, {
    // offset 0x000
    // QSSI Control 0
    REGMAP_32 (, {
        unsigned DSS: 4;
        unsigned FRF: 2;
        unsigned SPO: 1;
        unsigned SPH: 1;
        unsigned SCR: 8;
    }) CR0;

    // offset 0x004
    // QSSI Control 1
    REGMAP_32 (, {
        unsigned LBM: 1;
        unsigned SSE: 1;
        unsigned MS: 1;
        unsigned : 3;
        unsigned MODE: 2;
        unsigned DIR: 1;
        unsigned HSCLKEN: 1;
        unsigned FSSHLDFRM: 1;
        unsigned EOM: 1;
    }) CR1;

    // offset 0x008
    // QSSI Data
    REGMAP_32 (, {
            unsigned DATA: 16;
    }) DR;

    // offset 0x00C
    // QSSI Status
    REGMAP_32 (, {
        unsigned TFE: 1;
        unsigned TNF: 1;
        unsigned RNE: 1;
        unsigned RFF: 1;
        unsigned BSY: 1;
    }) SR;

    // offset 0x010
    // QSSI Clock Prescale
    REGMAP_32 (, {
        unsigned CPSDVSR: 8;
    }) CPSR;

    uint32_t IM;        // QSSI Interrupt Mask
    uint32_t RIS;       // QSSI Raw Interrupt Status
    uint32_t MIS;       // QSSI Masked Interrupt Status
    uint32_t ICR;       // QSSI Interrupt Clear
    uint32_t DMACTL;    // QSSI DMA Control
    char _reserved_00[0xF98];

    // offset 0xFC0
    // QSSI Peripheral Properties
    uint32_t PP;

    char _reserved_01[4];

    // offset 0xFC8
    // QSSI Clock Configuration
    REGMAP_32 (, {
        unsigned CS: 4;
    }) CC;

    char _reserved_02[4];
    // offset 0xFD0
    uint32_t PeriphID4;
    uint32_t PeriphID5;
    uint32_t PeriphID6;
    uint32_t PeriphID7;
    uint32_t PeriphID0;
    uint32_t PeriphID1;
    uint32_t PeriphID2;
    uint32_t PeriphID3;
    uint32_t PCellID0;
    uint32_t PCellID1;
    uint32_t PCellID2;
    uint32_t PCellID3;
})

#define SSI0    (*(volatile struct QSSI_MAP *) 0x40008000)
#define SSI1    (*(volatile struct QSSI_MAP *) 0x40009000)
#define SSI2    (*(volatile struct QSSI_MAP *) 0x4000A000)
#define SSI3    (*(volatile struct QSSI_MAP *) 0x4000B000)

REGMAP_32 (RCGCSSI_MAP, {
    unsigned EN_SSI0: 1;
    unsigned EN_SSI1: 1;
    unsigned EN_SSI2: 1;
    unsigned EN_SSI3: 1;
});
#define RCGCSSI (*(volatile union RCGCSSI_MAP *)0x400FE61C)

#endif //TM4C_SSI_H
