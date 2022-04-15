/**
 * System Registers
 * @author Robert J. Rouquette
 * @date 2022-04-14
 */

#ifndef TM4C_SYS_H
#define TM4C_SYS_H

#include <stdint.h>

union RCGCGPIO_MAP{
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

union RCGCSSI_MAP{
    struct {
        unsigned EN_SSI0: 1;
        unsigned EN_SSI1: 1;
        unsigned EN_SSI2: 1;
        unsigned EN_SSI3: 1;
    };
    uint32_t raw;
};
#define RCGCSSI (*(volatile union RCGCSSI_MAP *)0x400FE61C)

#endif //TM4C_SYS_H
