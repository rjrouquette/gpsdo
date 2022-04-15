//
// Created by robert on 4/14/22.
//

#ifndef GPSDO_GPIO_H
#define GPSDO_GPIO_H

#include <stdint.h>

typedef struct {
    uint32_t DATA[256];
    // offset 0x0400
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
    char _reserved_02[0xDC];
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
} GPIO_t;

#define GPIOA   (*(volatile GPIO_t *) 0x40058000)
#define GPIOB   (*(volatile GPIO_t *) 0x40059000)
#define GPIOC   (*(volatile GPIO_t *) 0x4005A000)
#define GPIOD   (*(volatile GPIO_t *) 0x4005B000)
#define GPIOE   (*(volatile GPIO_t *) 0x4005C000)
#define GPIOF   (*(volatile GPIO_t *) 0x4005D000)
#define GPIOG   (*(volatile GPIO_t *) 0x4005E000)
#define GPIOH   (*(volatile GPIO_t *) 0x4005F000)
#define GPIOJ   (*(volatile GPIO_t *) 0x40060000)
#define GPIOK   (*(volatile GPIO_t *) 0x40061000)
#define GPIOL   (*(volatile GPIO_t *) 0x40062000)
#define GPIOM   (*(volatile GPIO_t *) 0x40063000)
#define GPION   (*(volatile GPIO_t *) 0x40064000)
#define GPIOP   (*(volatile GPIO_t *) 0x40065000)
#define GPIOQ   (*(volatile GPIO_t *) 0x40066000)

#endif //GPSDO_GPIO_H
 /*
  * #define GPIO_O_DATA             0x00000000  // GPIO Data
#define GPIO_O_DIR              0x00000400  // GPIO Direction
#define GPIO_O_IS               0x00000404  // GPIO Interrupt Sense
#define GPIO_O_IBE              0x00000408  // GPIO Interrupt Both Edges
#define GPIO_O_IEV              0x0000040C  // GPIO Interrupt Event
#define GPIO_O_IM               0x00000410  // GPIO Interrupt Mask
#define GPIO_O_RIS              0x00000414  // GPIO Raw Interrupt Status
#define GPIO_O_MIS              0x00000418  // GPIO Masked Interrupt Status
#define GPIO_O_ICR              0x0000041C  // GPIO Interrupt Clear
#define GPIO_O_AFSEL            0x00000420  // GPIO Alternate Function Select
#define GPIO_O_DR2R             0x00000500  // GPIO 2-mA Drive Select
#define GPIO_O_DR4R             0x00000504  // GPIO 4-mA Drive Select
#define GPIO_O_DR8R             0x00000508  // GPIO 8-mA Drive Select
#define GPIO_O_ODR              0x0000050C  // GPIO Open Drain Select
#define GPIO_O_PUR              0x00000510  // GPIO Pull-Up Select
#define GPIO_O_PDR              0x00000514  // GPIO Pull-Down Select
#define GPIO_O_SLR              0x00000518  // GPIO Slew Rate Control Select
#define GPIO_O_DEN              0x0000051C  // GPIO Digital Enable
#define GPIO_O_LOCK             0x00000520  // GPIO Lock
#define GPIO_O_CR               0x00000524  // GPIO Commit
#define GPIO_O_AMSEL            0x00000528  // GPIO Analog Mode Select
#define GPIO_O_PCTL             0x0000052C  // GPIO Port Control
#define GPIO_O_ADCCTL           0x00000530  // GPIO ADC Control
#define GPIO_O_DMACTL           0x00000534  // GPIO DMA Control
#define GPIO_O_SI               0x00000538  // GPIO Select Interrupt
#define GPIO_O_DR12R            0x0000053C  // GPIO 12-mA Drive Select
#define GPIO_O_WAKEPEN          0x00000540  // GPIO Wake Pin Enable
#define GPIO_O_WAKELVL          0x00000544  // GPIO Wake Level
#define GPIO_O_WAKESTAT         0x00000548  // GPIO Wake Status
#define GPIO_O_PP               0x00000FC0  // GPIO Peripheral Property
#define GPIO_O_PC               0x00000FC4  // GPIO Peripheral Configuration
  */