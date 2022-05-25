//
// Created by robert on 4/19/22.
//

#ifndef TM4C_TIMER_H
#define TM4C_TIMER_H

#include "register.h"

REGMAP_32 (GPTM_MODE_MAP, {
    unsigned MR: 2;
    unsigned CMR: 1;
    unsigned AMS: 1;
    unsigned CDIR: 1;
    unsigned MIE: 1;
    unsigned WOT: 1;
    unsigned SNAPS: 1;
    unsigned ILD: 1;
    unsigned WMIE: 1;
    unsigned TAMRSU: 1;
    unsigned PLO: 1;
    unsigned CINTD: 1;
    unsigned TCACT: 3;
});

PAGE_MAP(GPTM_MAP, {
    // offset 0x000
    // GPTM Configuration
    REGMAP_32 (, {
        unsigned GPTMCFG: 4;
    }) CFG;

    union GPTM_MODE_MAP TAMR;      // GPTM Timer A Mode
    union GPTM_MODE_MAP TBMR;      // GPTM Timer B Mode

    // offset 0x00C
    // GPTM Control
    REGMAP_32 (, {
        unsigned TAEN: 1;
        unsigned TASTALL: 1;
        unsigned TAEVENT: 2;
        unsigned RTCEN: 1;
        unsigned TAOTE: 1;
        unsigned TAPWML: 1;
        unsigned : 1;
        unsigned TBEN: 1;
        unsigned TBSTALL: 1;
        unsigned TBEVENT: 1;
        unsigned : 1;
        unsigned TBOTE: 1;
        unsigned TBPWML: 1;
    }) CTL;

    uint32_t SYNC;      // GPTM Synchronize
    char _reserved_0[0x004];

    // offset 0x018
    // GPTM Interrupt Mask
    REGMAP_32 (, {
            unsigned TATO: 1;
            unsigned CAM: 1;
            unsigned CAE: 1;
            unsigned RTC: 1;
            unsigned TAM: 1;
            unsigned DMAA: 1;
            unsigned : 2;
            unsigned TBTO: 1;
            unsigned CBM: 1;
            unsigned CBE: 1;
            unsigned : 1;
            unsigned TBM: 1;
            unsigned DMAB: 1;
    }) IMR;

    // offset 0x01C
    // GPTM Raw Interrupt Status
    REGMAP_32 (, {
        unsigned TATO: 1;
        unsigned CAM: 1;
        unsigned CAE: 1;
        unsigned RTC: 1;
        unsigned TAM: 1;
        unsigned DMAA: 1;
        unsigned : 2;
        unsigned TBTO: 1;
        unsigned CBM: 1;
        unsigned CBE: 1;
        unsigned : 1;
        unsigned TBM: 1;
        unsigned DMAB: 1;
    }) RIS;

    // offset 0x020
    // GPTM Masked Interrupt Status
    REGMAP_32(, {
        unsigned TATO: 1;
        unsigned CAM: 1;
        unsigned CAE: 1;
        unsigned RTC: 1;
        unsigned TAM: 1;
        unsigned DMAA: 1;
        unsigned : 2;
        unsigned TBTO: 1;
        unsigned CBM: 1;
        unsigned CBE: 1;
        unsigned : 1;
        unsigned TBM: 1;
        unsigned DMAB: 1;
    }) MIS;

    // offset 0x024
    // GPTM Interrupt Clear
    REGMAP_32(, {
        unsigned TATO: 1;
        unsigned CAM: 1;
        unsigned CAE: 1;
        unsigned RTC: 1;
        unsigned TAM: 1;
        unsigned DMAA: 1;
        unsigned : 2;
        unsigned TBTO: 1;
        unsigned CBM: 1;
        unsigned CBE: 1;
        unsigned : 1;
        unsigned TBM: 1;
        unsigned DMAB: 1;
    }) ICR;

    uint32_t TAILR;     // GPTM Timer A Interval Load
    uint32_t TBILR;     // GPTM Timer B Interval Load
    uint32_t TAMATCHR;  // GPTM Timer A Match
    uint32_t TBMATCHR;  // GPTM Timer B Match
    uint32_t TAPR;      // GPTM Timer A Prescale
    uint32_t TBPR;      // GPTM Timer B Prescale
    uint32_t TAPMR;     // GPTM Timer A Prescale Match
    uint32_t TBPMR;     // GPTM Timer B Prescale Match

    // offset 0x048
    // GPTM Timer A
    REGMAP_32(, {
        uint16_t LO: 16;
    }) TAR;

    // offset 0x04C
    // GPTM Timer B
    REGMAP_32(, {
        uint16_t LO: 16;
    }) TBR;

    // offset 0x050
    // GPTM Timer A Value
    REGMAP_32(, {
        uint16_t LO: 16;
    }) TAV;

    // offset 0x054
    // GPTM Timer B Value
    REGMAP_32(, {
        uint16_t LO: 16;
    }) TBV;

    uint32_t RTCPD;     // GPTM RTC Predivide
    uint32_t TAPS;      // GPTM Timer A Prescale Snapshot
    uint32_t TBPS;      // GPTM Timer B Prescale Snapshot
    char _reserved_1[0x008];
    uint32_t DMAEV;     // GPTM DMA Event
    uint32_t ADCEV;     // GPTM ADC Event
    char _reserved_2[0xF4C];
    // offset 0xFC0
    uint32_t PP;  // Timer Peripheral Property
    uint32_t CC;  // Timer Clock Configuration
    // reserved space
    char _reserved_3[0x008];
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

#define GPTM0   (*(volatile struct GPTM_MAP *) 0x40030000)
#define GPTM1   (*(volatile struct GPTM_MAP *) 0x40031000)
#define GPTM2   (*(volatile struct GPTM_MAP *) 0x40032000)
#define GPTM3   (*(volatile struct GPTM_MAP *) 0x40033000)
#define GPTM4   (*(volatile struct GPTM_MAP *) 0x40034000)
#define GPTM5   (*(volatile struct GPTM_MAP *) 0x40035000)
#define GPTM6   (*(volatile struct GPTM_MAP *) 0x400E0000)
#define GPTM7   (*(volatile struct GPTM_MAP *) 0x400E1000)

REGMAP_32 (RCGCTIMER_MAP, {
    unsigned EN_GPTM0: 1;
    unsigned EN_GPTM1: 1;
    unsigned EN_GPTM2: 1;
    unsigned EN_GPTM3: 1;
    unsigned EN_GPTM4: 1;
    unsigned EN_GPTM5: 1;
    unsigned EN_GPTM6: 1;
    unsigned EN_GPTM7: 1;
});
#define RCGCTIMER (*(volatile union RCGCTIMER_MAP *)0x400FE604)

#endif //TM4C_TIMER_H
