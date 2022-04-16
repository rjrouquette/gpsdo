//
// Created by robert on 4/16/22.
//

#ifndef TM4C_UART_H
#define TM4C_UART_H

#include <stdint.h>
#include "gpio.h"

struct UART_MAP {
    // offset 0x000
    union {
        struct {
            unsigned DATA: 8;
            unsigned FE: 1;
            unsigned PE: 1;
            unsigned BE: 1;
            unsigned OE: 1;
        };
        uint32_t raw;
    } DR;               // UART Data Register

    union {
        struct {
            unsigned FE: 1;
            unsigned PE: 1;
            unsigned BE: 1;
            unsigned OE: 1;
        };
        uint32_t raw;
    } ECR;              // UART Error Clear

    char _reserved_00[0x010];
    // offset 0x018
    union {
        struct {
            unsigned CTS: 1;
            unsigned DSR: 1;
            unsigned DCD: 1;
            unsigned BUSY: 1;
            unsigned RXFE: 1;
            unsigned TXFF: 1;
            unsigned RXFF: 1;
            unsigned TXFE: 1;
            unsigned RI: 1;
        };
        uint32_t raw;
    } FR;               // UART Flags

    char _reserved_01[0x004];
    // offset 0x020
    union {
        struct {
            unsigned ILPDVSR: 8;
        };
        uint32_t raw;
    } ILPR;             // UART IrDA Low-Power Register

    union {
        struct {
            unsigned DIVINT: 16;
        };
        uint32_t raw;
    } IBRD;             // UART Integer Baud-Rate Divisor

    union {
        struct {
            unsigned DIVFRAC: 6;
        };
        uint32_t raw;
    } FBRD;             // UART Fractional Baud-Rate Divisor

    union {
        struct {
            unsigned BRK: 1;
            unsigned PEN: 1;
            unsigned EPS: 1;
            unsigned STP2: 1;
            unsigned FEN: 1;
            unsigned WLEN: 2;
            unsigned SPS: 1;
        };
        uint32_t raw;
    } UARTLCRH;         // UART UART Line Control

    uint32_t IM;        // UART Interrupt Mask
    uint32_t RIS;       // UART Raw Interrupt Status
    uint32_t MIS;       // UART Masked Interrupt Status
    uint32_t ICR;       // UART Interrupt Clear
    uint32_t DMACTL;    // UART DMA Control
    char _reserved_00[0xF98];
    // offset 0xFC0
    uint32_t PP;    // UART Peripheral Properties
    char _reserved_01[4];
    // offset 0xFC8
    union {
        struct {
            enum SSI_CLK_SRC_e CS: 4;
        };
        uint32_t raw;
    } CC;               // UART Clock Configuration Configuration

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
_Static_assert(sizeof(struct UART_MAP) == 4096, "UART_MAP must be 4096 bytes");

#define UART0    (*(volatile struct UART_MAP *) 0x4000C000)
#define UART1    (*(volatile struct UART_MAP *) 0x4000D000)
#define UART2    (*(volatile struct UART_MAP *) 0x4000E000)
#define UART3    (*(volatile struct UART_MAP *) 0x4000F000)
#define UART4    (*(volatile struct UART_MAP *) 0x40010000)
#define UART5    (*(volatile struct UART_MAP *) 0x40011000)
#define UART6    (*(volatile struct UART_MAP *) 0x40012000)
#define UART7    (*(volatile struct UART_MAP *) 0x40013000)

#endif //TM4C_UART_H
