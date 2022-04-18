//
// Created by robert on 4/17/22.
//

#ifndef TM4C_ADC_H
#define TM4C_ADC_H

#include <stdint.h>
#include "gpio.h"

enum ADC_SS_TRIGGER {
    ADC_SS_TRIG_SOFT = 0x0,     // Software Triggered
    ADC_SS_TRIG_AC0 = 0x1,      // Analog Comparator 0 (AC0.CTL)
    ADC_SS_TRIG_AC1 = 0x2,      // Analog Comparator 1 (AC1.CTL)
    ADC_SS_TRIG_AC2 = 0x3,      // Analog Comparator 2 (AC2.CTL)
    ADC_SS_TRIG_GPIO = 0x4,     // GPIO Interrupt
    ADC_SS_TRIG_TIMER = 0x5,    // Timer Event (GPTMCTL)
    ADC_SS_TRIG_PWM0 = 0x6,     // PWM 0 (PWM0.INTEN)
    ADC_SS_TRIG_PWM1 = 0x7,     // PWM 3 (PWM1.INTEN)
    ADC_SS_TRIG_PWM2 = 0x8,     // PWM 3 (PWM2.INTEN)
    ADC_SS_TRIG_PWM3 = 0x9,     // PWM 3 (PWM3.INTEN)

    ADC_SS_TRIG_NONE = 0xE,     // No Triggers Permitted
    ADC_SS_TRIG_RUN = 0xF       // Continuous Operation (Free-Running)
};

enum ADC_CLK_SRC {
    ADC_CLK_PLL = 0x0,          // Use PLL with divider
    ADC_CLK_ALT = 0x1,          // Use ALT clock
    ADC_CLK_MOSC = 0x2          // Use MOSC
};

struct ADC_SS_MAP {
    uint32_t MUX;           // ADC Sample Sequence Input Multiplexer Select
    uint32_t CTL;           // ADC Sample Sequence Control
    uint32_t FIFO;          // ADC Sample Sequence Result FIFO
    uint32_t FSTAT;         // ADC Sample Sequence FIFO Status
    uint32_t OP;            // ADC Sample Sequence Operation
    uint32_t DC;            // ADC Sample Sequence Digital Comparator Select
    uint32_t EMUX;          // ADC Sample Sequence Extended Input Multiplexer Select
    uint32_t TSH;           // ADC Sample Sequence Sample and Hold Time
};
_Static_assert(sizeof(struct ADC_SS_MAP) == 32, "ADC_SS_MAP must be 32 bytes");

struct ADC_MAP {
    // offset 0x000
    union {
        struct {
            unsigned ASEN0: 1;
            unsigned ASEN1: 1;
            unsigned ASEN2: 1;
            unsigned ASEN3: 1;
            unsigned _reserved_00: 4;
            unsigned ADEN0: 1;
            unsigned ADEN1: 1;
            unsigned ADEN2: 1;
            unsigned ADEN3: 1;
            unsigned _reserved_01: 4;
            unsigned BUSY: 1;
        };
        uint32_t raw;
    } ACTSS;                // ADC Active Sample Sequencer
    uint32_t RIS;           // ADC Raw Interrupt Status
    uint32_t IM;            // ADC Interrupt Mask
    uint32_t ISC;           // ADC Interrupt Status and Clear
    uint32_t OSTAT;         // ADC Overflow Status
    union {
        struct {
            enum ADC_SS_TRIGGER EM0: 4;
            enum ADC_SS_TRIGGER EM1: 4;
            enum ADC_SS_TRIGGER EM2: 4;
            enum ADC_SS_TRIGGER EM3: 4;
        };
        uint32_t raw;
    } EMUX;                 // ADC Event Multiplexer
    uint32_t USTAT;         // ADC Underflow Status
    uint32_t TSSEL;         // ADC Trigger Source Select
    uint32_t SSPRI;         // ADC Sample Sequence Priority
    uint32_t SPC;           // ADC Sample Phase Control
    uint32_t PSSI;          // ADC Processor Sample Sequence Initiate
    char _reserved_00[0x004];
    union {
        struct {
            unsigned AVG: 3;
        };
        uint32_t raw;
    } SAC;                  // ADC Sample Averaging Control
    uint32_t DCISC;         // ADC Digital Comparator Interrupt Status and Clear
    uint32_t CTL;           // ADC Control
    char _reserved_01[0x004];
    // offset 0x040
    struct ADC_SS_MAP SS0;  // ADC Sample Sequence 0
    // offset 0x060
    struct ADC_SS_MAP SS1;  // ADC Sample Sequence 1
    // offset 0x080
    struct ADC_SS_MAP SS2;  // ADC Sample Sequence 2
    // offset 0x0A0
    struct ADC_SS_MAP SS3;  // ADC Sample Sequence 3
    char _reserved_02[0xC40];
    // offset 0xD00
    uint32_t DCRIC;         // ADC Digital Comparator Reset Initial Conditions
    char _reserved_03[0x0FC];
    // offset 0xE00
    uint32_t DCCTL0;        // ADC Digital Comparator Control 0
    uint32_t DCCTL1;        // ADC Digital Comparator Control 1
    uint32_t DCCTL2;        // ADC Digital Comparator Control 2
    uint32_t DCCTL3;        // ADC Digital Comparator Control 3
    uint32_t DCCTL4;        // ADC Digital Comparator Control 4
    uint32_t DCCTL5;        // ADC Digital Comparator Control 5
    uint32_t DCCTL6;        // ADC Digital Comparator Control 6
    uint32_t DCCTL7;        // ADC Digital Comparator Control 7
    char _reserved_04[0x020];
    // offset 0xE40
    uint32_t DCCMP0;        // ADC Digital Comparator Range 0
    uint32_t DCCMP1;        // ADC Digital Comparator Range 1
    uint32_t DCCMP2;        // ADC Digital Comparator Range 2
    uint32_t DCCMP3;        // ADC Digital Comparator Range 3
    uint32_t DCCMP4;        // ADC Digital Comparator Range 4
    uint32_t DCCMP5;        // ADC Digital Comparator Range 5
    uint32_t DCCMP6;        // ADC Digital Comparator Range 6
    uint32_t DCCMP7;        // ADC Digital Comparator Range 7
    char _reserved_05[0x160];
    // offset 0xFC0
    uint32_t PP;            // ADC Peripheral Properties
    uint32_t PC;            // ADC Peripheral Configuration
    union {
        struct {
            enum ADC_CLK_SRC CS: 4;
            unsigned CLKDIV: 6;
        };
        uint32_t raw;
    } CC;                   // ADC Clock Configuration
    char _reserved_06[0x004];
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
_Static_assert(sizeof(struct ADC_MAP) == 4096, "ADC_MAP must be 4096 bytes");

#define ADC0    (*(volatile struct ADC_MAP *) 0x40038000)
#define ADC1    (*(volatile struct ADC_MAP *) 0x40039000)

union RCGCADC_MAP {
    struct {
        unsigned EN_ADC0: 1;
        unsigned EN_ADC1: 1;
    };
    uint32_t raw;
};
#define RCGCADC (*(volatile union RCGCADC_MAP *)0x400FE638)

#endif //TM4C_ADC_H
