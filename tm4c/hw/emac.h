/**
 * Ethernet MAC Register Mapping
 * @author Robert J. Rouquette
 * @date 2022-04-24
 */

#ifndef TM4C_EMAC_H
#define TM4C_EMAC_H

#include <stdint.h>

enum EMAC_SADDR {
    EMAC_SADDR_INS0 = 0x02,
    EMAC_SADDR_REP0 = 0x03,
    EMAC_SADDR_INS1 = 0x06,
    EMAC_SADDR_REP1 = 0x07,
};

struct EMAC_MAP {
    // offset 0x000
    union {
        struct {
            unsigned PRELEN: 2;
            unsigned RE: 1;
            unsigned TE: 1;
            unsigned DC: 1;
            unsigned BL: 2;
            unsigned ACS: 1;
            unsigned : 1;
            unsigned DR: 1;
            unsigned IPC: 1;
            unsigned DUPM: 1;
            unsigned LOOPBM: 1;
            unsigned DRO: 1;
            unsigned FES: 1;
            unsigned PS: 1;
            unsigned DISCRS: 1;
            unsigned IFG: 1;
            unsigned JFEN: 1;
            unsigned : 1;
            unsigned JD: 1;
            unsigned WDDIS: 1;
            unsigned : 1;
            unsigned CST: 1;
            unsigned : 1;
            unsigned TWOKPEN: 1;
            unsigned SADDR: 1;
        };
        uint32_t raw;
    } CFG;
    // offset 0x400
    uint32_t DIR;       // GPIO Direction
    uint32_t IS;        // GPIO Interrupt Sense
    uint32_t IBE;       // GPIO Interrupt Both Edges
    uint32_t IEV;       // GPIO Interrupt Event
    uint32_t IM;        // GPIO Interrupt Mask
    uint32_t RIS;       // GPIO Masked Interrupt Status
    uint32_t MIS;       // GPIO Masked Interrupt Status
    uint32_t ICR;       // GPIO Interrupt Clear
    uint32_t AFSEL;     // GPIO Alternate Function Select
    // reserved space
    char _reserved_00[0xDC];
    // offset 0x500
    uint32_t DR2R;      // GPIO 2-mA Drive Select
    uint32_t DR4R;      // GPIO 4-mA Drive Select
    uint32_t DR8R;      // GPIO 8-mA Drive Select
    uint32_t ODR;       // GPIO Open Drain Select
    uint32_t PUR;       // GPIO Pull-Up Select
    uint32_t PDR;       // GPIO Pull-Down Select
    uint32_t SLR;       // GPIO Slew Rate Control Select
    uint32_t DEN;       // GPIO Digital Enable
    uint32_t LOCK;      // GPIO Lock
    uint32_t CR;        // GPIO Commit
    uint32_t AMSEL;     // GPIO Analog Mode Select
    uint32_t PCTL;      // GPIO Port Control
    uint32_t ADCCTL;    // GPIO ADC Control
    uint32_t DMACTL;    // GPIO DMA Control
    uint32_t SI;        // GPIO Select Interrupt
    uint32_t DR12R;     // GPIO 12-mA Drive Select
    uint32_t WAKEPEN;   // GPIO Wake Pin Enable
    uint32_t WAKELVL;   // GPIO Wake Level
    uint32_t WAKESTAT;  // GPIO Wake Status
    // reserved space
    char _reserved_01[0xA74];
    // offset 0xFC0
    uint32_t PP;  // GPIO Peripheral Property
    uint32_t PC;  // GPIO Peripheral Configuration
    // reserved space
    char _reserved_02[0x8];
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
_Static_assert(sizeof(struct GPIO_MAP) == 4096, "GPIO_MAP must be 4096 bytes");

#endif //TM4C_EMAC_H
