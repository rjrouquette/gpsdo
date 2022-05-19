/**
 * ADC Register Mapping
 * @author Robert J. Rouquette
 * @date 2022-04-17
 */

#ifndef TM4C_ADC_H
#define TM4C_ADC_H

#include "register.h"
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

enum ADC_TSH_WIDTH {
    ADC_TSH_4 = 0x0,            // 4 clock cycles
    ADC_TSH_8 = 0x2,            // 8 clock cycles
    ADC_TSH_16 = 0x4,           // 16 clock cycles
    ADC_TSH_32 = 0x6,           // 32 clock cycles
    ADC_TSH_64 = 0x8,           // 64 clock cycles
    ADC_TSH_128 = 0xA,          // 128 clock cycles
    ADC_TSH_256 = 0xC,          // 256 clock cycles
};

enum ADC_CLK_SRC {
    ADC_CLK_PLL = 0x0,          // Use PLL with divider
    ADC_CLK_ALT = 0x1,          // Use ALT clock
    ADC_CLK_MOSC = 0x2          // Use MOSC
};

struct ADC_SS_MAP {
    // offset 0x000
    // ADC Sample Sequence Input Multiplexer Select
    REGMAP_32 (, {
        unsigned MUX0: 4;
        unsigned MUX1: 4;
        unsigned MUX2: 4;
        unsigned MUX3: 4;
        unsigned MUX4: 4;
        unsigned MUX5: 4;
        unsigned MUX6: 4;
        unsigned MUX7: 4;
    }) MUX;

    // offset 0x004
    // ADC Sample Sequence Control
    REGMAP_32 (, {
        unsigned D0: 1;
        unsigned END0: 1;
        unsigned IE0: 1;
        unsigned TS0: 1;
        unsigned D1: 1;
        unsigned END1: 1;
        unsigned IE1: 1;
        unsigned TS1: 1;
        unsigned D2: 1;
        unsigned END2: 1;
        unsigned IE2: 1;
        unsigned TS2: 1;
        unsigned D3: 1;
        unsigned END3: 1;
        unsigned IE3: 1;
        unsigned TS3: 1;
        unsigned D4: 1;
        unsigned END4: 1;
        unsigned IE4: 1;
        unsigned TS4: 1;
        unsigned D5: 1;
        unsigned END5: 1;
        unsigned IE5: 1;
        unsigned TS5: 1;
        unsigned D6: 1;
        unsigned END6: 1;
        unsigned IE6: 1;
        unsigned TS6: 1;
        unsigned D7: 1;
        unsigned END7: 1;
        unsigned IE7: 1;
        unsigned TS7: 1;
    }) CTL;

    // offset 0x008
    // ADC Sample Sequence Result FIFO
    REGMAP_32 (, {
        unsigned DATA: 12;
    }) FIFO;

    // offset 0x00C
    // ADC Sample Sequence FIFO Status
    REGMAP_32 (, {
        unsigned TPTR: 4;
        unsigned HPTR: 4;
        unsigned EMPTY: 1;
        unsigned : 3;
        unsigned FULL: 1;
    }) FSTAT;

    // offset 0x010
    // ADC Sample Sequence Operation
    REGMAP_32 (, {
        unsigned S0DCOP: 1;
        unsigned : 3;
        unsigned S1DCOP: 1;
        unsigned : 3;
        unsigned S2DCOP: 1;
        unsigned : 3;
        unsigned S3DCOP: 1;
        unsigned : 3;
        unsigned S4DCOP: 1;
        unsigned : 3;
        unsigned S5DCOP: 1;
        unsigned : 3;
        unsigned S6DCOP: 1;
        unsigned : 3;
        unsigned S7DCOP: 1;
        unsigned : 3;
    }) OP;

    // offset 0x014
    // ADC Sample Sequence Digital Comparator Select
    REGMAP_32 (, {
        unsigned S0DCSEL: 4;
        unsigned S1DCSEL: 4;
        unsigned S2DCSEL: 4;
        unsigned S3DCSEL: 4;
        unsigned S4DCSEL: 4;
        unsigned S5DCSEL: 4;
        unsigned S6DCSEL: 4;
        unsigned S7DCSEL: 4;
    }) DC;

    // offset 0x018
    // ADC Sample Sequence Extended Input Multiplexer Select
    REGMAP_32 (, {
        unsigned EMUX0: 1;
        unsigned : 3;
        unsigned EMUX1: 1;
        unsigned : 3;
        unsigned EMUX2: 1;
        unsigned : 3;
        unsigned EMUX3: 1;
        unsigned : 3;
        unsigned EMUX4: 1;
        unsigned : 3;
        unsigned EMUX5: 1;
        unsigned : 3;
        unsigned EMUX6: 1;
        unsigned : 3;
        unsigned EMUX7: 1;
        unsigned : 3;
    }) EMUX;

    // offset 0x01C
    // ADC Sample Sequence Sample and Hold Time
    REGMAP_32 (, {
        unsigned TSH0: 4;
        unsigned TSH1: 4;
        unsigned TSH2: 4;
        unsigned TSH3: 4;
        unsigned TSH4: 4;
        unsigned TSH5: 4;
        unsigned TSH6: 4;
        unsigned TSH7: 4;
    }) TSH;
};
_Static_assert(sizeof(struct ADC_SS_MAP) == 32, "ADC_SS_MAP must be 32 bytes");

PAGE_MAP (ADC_MAP, {
    // offset 0x000
    // ADC Active Sample Sequencer
    REGMAP_32 (, {
        unsigned ASEN0: 1;
        unsigned ASEN1: 1;
        unsigned ASEN2: 1;
        unsigned ASEN3: 1;
        unsigned : 4;
        unsigned ADEN0: 1;
        unsigned ADEN1: 1;
        unsigned ADEN2: 1;
        unsigned ADEN3: 1;
        unsigned : 4;
        unsigned BUSY: 1;
    }) ACTSS;

    // offset 0x004
    // ADC Raw Interrupt Status
    REGMAP_32 (, {
        unsigned INR0: 1;
        unsigned INR1: 1;
        unsigned INR2: 1;
        unsigned INR3: 1;
        unsigned : 4;
        unsigned DMAINR0: 1;
        unsigned DMAINR1: 1;
        unsigned DMAINR2: 1;
        unsigned DMAINR3: 1;
        unsigned : 4;
        unsigned INRDC: 1;
    }) RIS;

    // offset 0x008
    // ADC Interrupt Mask
    REGMAP_32 (, {
        unsigned MASK0: 1;
        unsigned MASK1: 1;
        unsigned MASK2: 1;
        unsigned MASK3: 1;
        unsigned : 4;
        unsigned DMAMASK0: 1;
        unsigned DMAMASK1: 1;
        unsigned DMAMASK2: 1;
        unsigned DMAMASK3: 1;
        unsigned : 4;
        unsigned DCONSS0: 1;
        unsigned DCONSS1: 1;
        unsigned DCONSS2: 1;
        unsigned DCONSS3: 1;
    }) IM;

    // offset 0x00C
    // ADC Interrupt Status and Clear
    REGMAP_32 (, {
        unsigned IN0: 1;
        unsigned IN1: 1;
        unsigned IN2: 1;
        unsigned IN3: 1;
        unsigned : 4;
        unsigned DMAIN0: 1;
        unsigned DMAIN1: 1;
        unsigned DMAIN2: 1;
        unsigned DMAIN3: 1;
        unsigned : 4;
        unsigned DCINSS0: 1;
        unsigned DCINSS1: 1;
        unsigned DCINSS2: 1;
        unsigned DCINSS3: 1;
    }) ISC;

    // offset 0x010
    // ADC Overflow Status
    REGMAP_32 (, {
        unsigned OV0: 1;
        unsigned OV1: 1;
        unsigned OV2: 1;
        unsigned OV3: 1;
    }) OSTAT;

    // offset 0x014
    // ADC Event Multiplexer
    REGMAP_32 (, {
        unsigned EM0: 4;
        unsigned EM1: 4;
        unsigned EM2: 4;
        unsigned EM3: 4;
    }) EMUX;

    // offset 0x018
    uint32_t USTAT;         // ADC Underflow Status
    uint32_t TSSEL;         // ADC Trigger Source Select
    uint32_t SSPRI;         // ADC Sample Sequence Priority
    uint32_t SPC;           // ADC Sample Phase Control

    // offset 0x028
    // ADC Processor Sample Sequence Initiate
    REGMAP_32 (, {
        unsigned SS0: 1;
        unsigned SS1: 1;
        unsigned SS2: 1;
        unsigned SS3: 1;
        unsigned : 23;
        unsigned SYNCWAIT: 1;
        unsigned : 3;
        unsigned GSYNC: 1;
    }) PSSI;

    char _reserved_00[0x004];

    // offset 0x030
    // ADC Sample Averaging Control
    REGMAP_32 (, {
        unsigned AVG: 3;
    }) SAC;
    // offset 0x034
    uint32_t DCISC;         // ADC Digital Comparator Interrupt Status and Clear
    uint32_t CTL;           // ADC Control
    char _reserved_01[0x004];

    // offset 0x040
    // ADC Sample Sequence 0
    struct ADC_SS_MAP SS0;

    // offset 0x060
    // ADC Sample Sequence 1
    struct ADC_SS_MAP SS1;

    // offset 0x080
    // ADC Sample Sequence 2
    struct ADC_SS_MAP SS2;

    // offset 0x0A0
    // ADC Sample Sequence 3
    struct ADC_SS_MAP SS3;

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

    // offset 0xFC8
    // ADC Clock Configuration
    REGMAP_32 (, {
        unsigned CS: 4;
        unsigned CLKDIV: 6;
    }) CC;

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
})

#define ADC0    (*(volatile struct ADC_MAP *) 0x40038000)
#define ADC1    (*(volatile struct ADC_MAP *) 0x40039000)

REGMAP_32 (RCGCADC_MAP, {
    unsigned EN_ADC0: 1;
    unsigned EN_ADC1: 1;
});
#define RCGCADC (*(volatile union RCGCADC_MAP *)0x400FE638)

#endif //TM4C_ADC_H
