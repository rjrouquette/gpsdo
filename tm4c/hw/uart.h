//
// Created by robert on 4/16/22.
//

#ifndef TM4C_UART_H
#define TM4C_UART_H

#include "register.h"
#include "gpio.h"

enum UART_FIFO_INT_LVL {
    UART_FIFO_INT_LVL_1_8 = 0x0,
    UART_FIFO_INT_LVL_1_4 = 0x1,
    UART_FIFO_INT_LVL_1_2 = 0x2,
    UART_FIFO_INT_LVL_3_4 = 0x3,
    UART_FIFO_INT_LVL_7_8 = 0x4
};

PAGE_MAP (UART_MAP, {
    // offset 0x000
    // UART Data Register
    REGMAP_32 (, {
        unsigned DATA: 8;
        unsigned FE: 1;
        unsigned PE: 1;
        unsigned BE: 1;
        unsigned OE: 1;
    }) DR;

    // offset 0x004
    // UART Error Clear
    REGMAP_32 (, {
        unsigned FE: 1;
        unsigned PE: 1;
        unsigned BE: 1;
        unsigned OE: 1;
    }) ECR;

    char _reserved_00[0x010];

    // offset 0x018
    // UART Flags
    REGMAP_32 (, {
        unsigned CTS: 1;
        unsigned DSR: 1;
        unsigned DCD: 1;
        unsigned BUSY: 1;
        unsigned RXFE: 1;
        unsigned TXFF: 1;
        unsigned RXFF: 1;
        unsigned TXFE: 1;
        unsigned RI: 1;
    }) FR;

    char _reserved_01[0x004];

    // offset 0x020
    // UART IrDA Low-Power Register
    REGMAP_32 (, {
        unsigned ILPDVSR: 8;
    }) ILPR;

    // offset 0x024
    // UART Integer Baud-Rate Divisor
    REGMAP_32 (, {
        unsigned DIVINT: 16;
    }) IBRD;

    // offset 0x028
    // UART Fractional Baud-Rate Divisor
    REGMAP_32 (, {
        unsigned DIVFRAC: 6;
    }) FBRD;

    // offset 0x02C
    // UART Line Control
    REGMAP_32 (, {
        unsigned BRK: 1;
        unsigned PEN: 1;
        unsigned EPS: 1;
        unsigned STP2: 1;
        unsigned FEN: 1;
        unsigned WLEN: 2;
        unsigned SPS: 1;
    }) LCRH;

    // offset 0x030
    // UART Control
    REGMAP_32(, {
        unsigned UARTEN: 1;
        unsigned SIREN: 1;
        unsigned SIRLP: 1;
        unsigned SMART: 1;
        unsigned EOT: 1;
        unsigned HSE: 1;
        unsigned : 1;
        unsigned LBE: 1;
        unsigned TXE: 1;
        unsigned RXE: 1;
        unsigned DTR: 1;
        unsigned RTS: 1;
        unsigned : 2;
        unsigned RTSEN: 1;
        unsigned CTSEN: 1;
    }) CTL;

    // offset 0x034
    // UART Interrupt FIFO Level Select
    REGMAP_32(, {
        unsigned TXIFLSEL: 3;
        unsigned RXIFLSEL: 3;
    }) IFLS;

    // offset 0x038
    // UART Interrupt Mask
    REGMAP_32(, {
        unsigned RI: 1;
        unsigned CTS: 1;
        unsigned DCD: 1;
        unsigned DSR: 1;
        unsigned RX: 1;
        unsigned TX: 1;
        unsigned RT: 1;
        unsigned FE: 1;
        unsigned PE: 1;
        unsigned BE: 1;
        unsigned OE: 1;
        unsigned EOT: 1;
        unsigned BIT9: 1;
        unsigned : 3;
        unsigned DMARX: 1;
        unsigned DMATX: 1;
    }) IM;

    // offset 0x03C
    // UART Raw Interrupt Status
    REGMAP_32(, {
        unsigned RI: 1;
        unsigned CTS: 1;
        unsigned DCD: 1;
        unsigned DSR: 1;
        unsigned RX: 1;
        unsigned TX: 1;
        unsigned RT: 1;
        unsigned FE: 1;
        unsigned PE: 1;
        unsigned BE: 1;
        unsigned OE: 1;
        unsigned EOT: 1;
        unsigned BIT9: 1;
        unsigned : 3;
        unsigned DMARX: 1;
        unsigned DMATX: 1;
    }) RIS;

    // offset 0x040
    // UART Masked Interrupt Status
    REGMAP_32(, {
        unsigned RI: 1;
        unsigned CTS: 1;
        unsigned DCD: 1;
        unsigned DSR: 1;
        unsigned RX: 1;
        unsigned TX: 1;
        unsigned RT: 1;
        unsigned FE: 1;
        unsigned PE: 1;
        unsigned BE: 1;
        unsigned OE: 1;
        unsigned EOT: 1;
        unsigned BIT9: 1;
        unsigned : 3;
        unsigned DMARX: 1;
        unsigned DMATX: 1;
    }) MIS;

    // offset 0x044
    // UARTInterrupt Clear
    REGMAP_32(, {
            unsigned RI: 1;
            unsigned CTS: 1;
            unsigned DCD: 1;
            unsigned DSR: 1;
            unsigned RX: 1;
            unsigned TX: 1;
            unsigned RT: 1;
            unsigned FE: 1;
            unsigned PE: 1;
            unsigned BE: 1;
            unsigned OE: 1;
            unsigned EOT: 1;
            unsigned BIT9: 1;
            unsigned : 3;
            unsigned DMARX: 1;
            unsigned DMATX: 1;
    }) ICR;

    // offset 0x048
    // UART DMA Control
    REGMAP_32(, {
        unsigned RXDMAE: 1;
        unsigned TXDMAE: 1;
        unsigned DMAERR: 1;
    }) DMACTL;

    char _reserved_02[0x058];

    // offset 0x0A4
    // UART 9-Bit Self Address
    REGMAP_32(, {
        unsigned ADDR: 8;
        unsigned : 7;
        unsigned BIT9EN: 1;
    }) BIT9ADDR;

    // offset 0x0A8
    // UART 9-Bit Self Address Mask
    REGMAP_32(, {
        unsigned MASK: 8;
    }) BIT9AMASK;

    char _reserved_03[0xF14];

    // offset 0xFC0
    uint32_t PP;    // UART Peripheral Properties

    char _reserved_04[4];

    // offset 0xFC8
    // UART Clock Configuration
    REGMAP_32 (, {
        unsigned CS: 4;
    }) CC;

    char _reserved_05[4];

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

#define UART0   (*(volatile struct UART_MAP *) 0x4000C000)
#define UART1   (*(volatile struct UART_MAP *) 0x4000D000)
#define UART2   (*(volatile struct UART_MAP *) 0x4000E000)
#define UART3   (*(volatile struct UART_MAP *) 0x4000F000)
#define UART4   (*(volatile struct UART_MAP *) 0x40010000)
#define UART5   (*(volatile struct UART_MAP *) 0x40011000)
#define UART6   (*(volatile struct UART_MAP *) 0x40012000)
#define UART7   (*(volatile struct UART_MAP *) 0x40013000)


REGMAP_32 (RCGCUART_MAP, {
    unsigned EN_UART0: 1;
    unsigned EN_UART1: 1;
    unsigned EN_UART2: 1;
    unsigned EN_UART3: 1;
    unsigned EN_UART4: 1;
    unsigned EN_UART5: 1;
    unsigned EN_UART6: 1;
    unsigned EN_UART7: 1;
});
#define RCGCUART (*(volatile union RCGCUART_MAP *)0x400FE618)

#endif //TM4C_UART_H
