/**
 * GPIO Port Register Mapping
 * @author Robert J. Rouquette
 * @date 2022-04-14
 */

#ifndef TM4C_GPIO_H
#define TM4C_GPIO_H

#include "register.h"

PAGE_MAP(GPIO_MAP, {
    // offset 0x000
    uint32_t DATA[256];
    // offset 0x400
    uint32_t DIR;       // GPIO Direction
    uint32_t IS;        // GPIO Interrupt Sense
    uint32_t IBE;       // GPIO Interrupt Both Edges
    uint32_t IEV;       // GPIO Interrupt Event
    uint32_t IM;        // GPIO Interrupt Mask
    uint32_t RIS;       // GPIO Masked Interrupt Status
    uint32_t MIS;       // GPIO Masked Interrupt Status
    uint32_t ICR;       // GPIO Interrupt Clear

    // GPIO Alternate Function Select
    REGMAP_32(, {
        unsigned ALT0: 1;
        unsigned ALT1: 1;
        unsigned ALT2: 1;
        unsigned ALT3: 1;
        unsigned ALT4: 1;
        unsigned ALT5: 1;
        unsigned ALT6: 1;
        unsigned ALT7: 1;
    }) AFSEL;

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

    // GPIO Port Control
    REGMAP_32(, {
        unsigned PMC0: 4;
        unsigned PMC1: 4;
        unsigned PMC2: 4;
        unsigned PMC3: 4;
        unsigned PMC4: 4;
        unsigned PMC5: 4;
        unsigned PMC6: 4;
        unsigned PMC7: 4;
    }) PCTL;

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
})

#define PORTA   (*(volatile struct GPIO_MAP *) 0x40058000)
#define PORTB   (*(volatile struct GPIO_MAP *) 0x40059000)
#define PORTC   (*(volatile struct GPIO_MAP *) 0x4005A000)
#define PORTD   (*(volatile struct GPIO_MAP *) 0x4005B000)
#define PORTE   (*(volatile struct GPIO_MAP *) 0x4005C000)
#define PORTF   (*(volatile struct GPIO_MAP *) 0x4005D000)
#define PORTG   (*(volatile struct GPIO_MAP *) 0x4005E000)
#define PORTH   (*(volatile struct GPIO_MAP *) 0x4005F000)
#define PORTJ   (*(volatile struct GPIO_MAP *) 0x40060000)
#define PORTK   (*(volatile struct GPIO_MAP *) 0x40061000)
#define PORTL   (*(volatile struct GPIO_MAP *) 0x40062000)
#define PORTM   (*(volatile struct GPIO_MAP *) 0x40063000)
#define PORTN   (*(volatile struct GPIO_MAP *) 0x40064000)
#define PORTP   (*(volatile struct GPIO_MAP *) 0x40065000)
#define PORTQ   (*(volatile struct GPIO_MAP *) 0x40066000)

#define GPIO_LOCK_KEY (0x4C4F434B)

union RCGCGPIO_MAP {
    struct {
        unsigned EN_PORTA: 1;
        unsigned EN_PORTB: 1;
        unsigned EN_PORTC: 1;
        unsigned EN_PORTD: 1;
        unsigned EN_PORTE: 1;
        unsigned EN_PORTF: 1;
        unsigned EN_PORTG: 1;
        unsigned EN_PORTH: 1;
        unsigned EN_PORTJ: 1;
        unsigned EN_PORTK: 1;
        unsigned EN_PORTL: 1;
        unsigned EN_PORTM: 1;
        unsigned EN_PORTN: 1;
        unsigned EN_PORTP: 1;
        unsigned EN_PORTQ: 1;
    };
    uint32_t raw;
};
#define RCGCGPIO (*(volatile union RCGCGPIO_MAP *)0x400FE608)

#endif //TM4C_GPIO_H
