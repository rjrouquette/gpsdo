//
// Created by robert on 4/14/22.
//

#ifndef TM4C_SYS_H
#define TM4C_SYS_H

#include <stdint.h>

typedef union {
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
} RCGCGPIO_t;

#define RCGCGPIO (*(RCGCGPIO_t*)0x400FE608)

#endif //TM4C_SYS_H
