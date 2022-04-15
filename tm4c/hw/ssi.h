/**
 * Synchronous Serial Interfaces
 * @author Robert J. Rouquette
 * @date 2022-04-15
 */

#ifndef TM4C_SSI_H
#define TM4C_SSI_H

#include <stdint.h>
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

struct QSSI_MAP {
    // offset 0x000
    union {
        struct {
            enum SSI_DSS_e DSS: 4;
            uint32_t FRF: 2;
            uint32_t SPO: 1;
            uint32_t SPH: 1;
            uint32_t SCR: 8;
            uint32_t DIR: 1;
        };
        uint32_t raw;
    } CR0;              // QSSI Control 0

    union {
        struct {
            uint32_t LBM: 1;
            uint32_t SSE: 1;
            uint32_t MS: 1;
            uint32_t _reserved: 3;
            uint32_t MODE: 2;
            uint32_t DIR: 1;
            uint32_t HSCLKEN: 1;
            uint32_t FSSHLDFRM: 1;
            uint32_t EOM: 1;
        };
        uint32_t raw;
    } CR1;              // QSSI Control 1

    union {
        struct {
            uint32_t DATA: 16;
        };
        uint32_t raw;
    } DR;               // QSSI Data

    union {
        struct {
            uint32_t TFE: 1;
            uint32_t TNF: 1;
            uint32_t RNE: 1;
            uint32_t RFF: 1;
            uint32_t BSY: 1;
        };
        uint32_t raw;
    } SR;               // QSSI Status

    union {
        struct {
            uint32_t CPSDVSR: 8;
        };
        uint32_t raw;
    } CPSR;             // QSSI Clock Prescale

    uint32_t IM;        // QSSI Interrupt Mask
    uint32_t RIS;       // QSSI Raw Interrupt Status
    uint32_t MIS;       // QSSI Masked Interrupt Status
    uint32_t ICR;       // QSSI Interrupt Clear
    uint32_t DMACTL;    // QSSI DMA Control
    char _reserved_00[0xF98];
    // offset 0xFC0
    uint32_t PP;    // QSSI Peripheral Properties
    char _reserved_01[4];
    // offset 0xFC8
    union {
        struct {
            enum SSI_CLK_SRC_e CS: 4;
        };
        uint32_t raw;
    } CC;               // QSSI Clock Configuration Configuration

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
};
_Static_assert(sizeof(struct QSSI_MAP) == 4096, "QSSI_MAP must be 4096 bytes");

#define SSI0    (*(volatile struct QSSI_MAP *) 0x40008000)
#define SSI1    (*(volatile struct QSSI_MAP *) 0x40009000)
#define SSI2    (*(volatile struct QSSI_MAP *) 0x4000A000)
#define SSI3    (*(volatile struct QSSI_MAP *) 0x4000B000)

#endif //TM4C_SSI_H
