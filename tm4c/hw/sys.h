/**
 * System Registers
 * @author Robert J. Rouquette
 * @date 2022-04-14
 */

#ifndef TM4C_SYS_H
#define TM4C_SYS_H

#include <stdint.h>

union RIS_MAP{
    struct {
        unsigned _reserved_0: 1;
        unsigned BORRIS: 1;
        unsigned _reserved_1: 1;
        unsigned MOFRIS: 1;
        unsigned _reserved_2: 2;
        unsigned PLLLRIS: 1;
        unsigned _reserved_3: 1;
        unsigned MOSCPUPRIS: 1;
    };
    uint32_t raw;
};
#define SYSRIS (*(volatile union RIS_MAP *)0x400FE050)

union MOSCCTL_MAP{
    struct {
        unsigned CVAL: 1;
        unsigned MOSCIM: 1;
        unsigned NOXTAL: 1;
        unsigned PWRDN: 1;
        unsigned OSCRNG: 1;
    };
    uint32_t raw;
};
#define MOSCCTL (*(volatile union MOSCCTL_MAP *)0x400FE07C)

union RSCLKCFG_MAP{
    struct {
        unsigned PSYSDIV: 10;
        unsigned OSYSDIV: 10;
        unsigned OSCSRC: 4;
        unsigned PLLSRC: 4;
        unsigned USEPLL: 1;
        unsigned ACG: 1;
        unsigned NEWFREQ: 1;
        unsigned MEMTIMU: 1;
    };
    uint32_t raw;
};
#define RSCLKCFG (*(volatile union RSCLKCFG_MAP *)0x400FE0B0)

union MEMTIM0_MAP{
    struct {
        unsigned FWS: 4;
        unsigned _reserved_0: 1;
        unsigned FBCE: 1;
        unsigned FBCHT: 4;
        unsigned _reserved_1: 6;
        unsigned EWS: 4;
        unsigned _reserved_2: 1;
        unsigned EBCE: 1;
        unsigned EBCHT: 4;
    };
    uint32_t raw;
};
#define MEMTIM0 (*(volatile union MEMTIM0_MAP *)0x400FE0C0)

union PLLFREQ0_MAP{
    struct {
        unsigned MINT: 10;
        unsigned MFRAC: 10;
        unsigned _reserved: 3;
        unsigned PLLPWR: 1;
    };
    uint32_t raw;
};
#define PLLFREQ0 (*(volatile union PLLFREQ0_MAP *)0x400FE160)

union PLLFREQ1_MAP{
    struct {
        unsigned N: 5;
        unsigned _reserved: 3;
        unsigned Q: 5;
    };
    uint32_t raw;
};
#define PLLFREQ1 (*(volatile union PLLFREQ1_MAP *)0x400FE164)

union PLLSTAT_MAP{
    struct {
        unsigned LOCK: 1;
    };
    uint32_t raw;
};
#define PLLSTAT (*(volatile union PLLSTAT_MAP *)0x400FE168)

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
