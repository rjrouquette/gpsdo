//
// Created by robert on 5/23/22.
//

#ifndef TM4C_I2C_H
#define TM4C_I2C_H

#include "register.h"
#include "gpio.h"

struct I2C_MASTER_MAP {
    // I2C Master Slave Address
    REGMAP_32(, {
        unsigned RX: 1;
        unsigned SA: 7;
    }) SA;

    // I2C Master Control/Status
    REGMAP_32(, {
        unsigned BUSY: 1;
        unsigned ERROR: 1;
        unsigned ADRACK: 1;
        unsigned DATACK: 1;
        unsigned ARBLST: 1;
        unsigned IDLE: 1;
        unsigned BUSBSY: 1;
        unsigned CLKTO: 1;
        unsigned : 22;
        unsigned ACTDMATX: 1;
        unsigned ACTDMARX: 1;
    }) CS;

    // I2C Master Data
    REGMAP_32(, {
        unsigned DATA: 8;
    }) DR;

    // I2C Master Timer Period
    REGMAP_32(, {
        unsigned TPR: 7;
        unsigned HR: 1;
        unsigned : 8;
        unsigned PULSEL: 3;
    }) TPR;

    // I2C Master Interrupt Mask
    REGMAP_32(, {
        unsigned IM: 1;
        unsigned CLK: 1;
        unsigned DMARX: 1;
        unsigned DMATX: 1;
        unsigned NACK: 1;
        unsigned START: 1;
        unsigned STOP: 1;
        unsigned ARBLOST: 1;
        unsigned TX: 1;
        unsigned RX: 1;
        unsigned TXFE: 1;
        unsigned RXFF: 1;
    }) IMR;

    // I2C Master Raw Interrupt Status
    REGMAP_32(, {
        unsigned IM: 1;
        unsigned CLK: 1;
        unsigned DMARX: 1;
        unsigned DMATX: 1;
        unsigned NACK: 1;
        unsigned START: 1;
        unsigned STOP: 1;
        unsigned ARBLOST: 1;
        unsigned TX: 1;
        unsigned RX: 1;
        unsigned TXFE: 1;
        unsigned RXFF: 1;
    }) RIS;

    // I2C Master Masked Interrupt Status
    REGMAP_32(, {
        unsigned IM: 1;
        unsigned CLK: 1;
        unsigned DMARX: 1;
        unsigned DMATX: 1;
        unsigned NACK: 1;
        unsigned START: 1;
        unsigned STOP: 1;
        unsigned ARBLOST: 1;
        unsigned TX: 1;
        unsigned RX: 1;
        unsigned TXFE: 1;
        unsigned RXFF: 1;
    }) MIS;

    // I2C Master Interrupt Clear
    REGMAP_32(, {
        unsigned IM: 1;
        unsigned CLK: 1;
        unsigned DMARX: 1;
        unsigned DMATX: 1;
        unsigned NACK: 1;
        unsigned START: 1;
        unsigned STOP: 1;
        unsigned ARBLOST: 1;
        unsigned TX: 1;
        unsigned RX: 1;
        unsigned TXFE: 1;
        unsigned RXFF: 1;
    }) ICR;

    // I2C Master Configuration
    REGMAP_32(, {
        unsigned LPBK: 1;
        unsigned : 3;
        unsigned MFE: 1;
        unsigned SFE: 1;
    }) CR;

    // I2C Master Clock Low Timeout Count
    REGMAP_32(, {
        unsigned CNTL: 8;
    }) CLKOCNT;

    uint32_t _reserved;

    // I2C Master Bus Monitor
    REGMAP_32(, {
        unsigned SCL: 1;
        unsigned SDA: 1;
    }) BMON;

    // I2C Master Burst Length
    REGMAP_32(, {
        unsigned CNTL: 8;
    }) BLEN;

    // I2C Master Burst Count
    REGMAP_32(, {
        unsigned CNTL: 8;
    }) BCNT;
};
_Static_assert(sizeof(struct I2C_MASTER_MAP) == 56, "ADC_SS_MAP must be 56 bytes");

struct I2C_SLAVE_MAP {
    // I2C Slave Own Address
    REGMAP_32(, {
        unsigned OAR: 7;
    }) OAR;

    // I2C Slave Control/Status
    REGMAP_32(, {
        unsigned RREQ: 1;
        unsigned TREQ: 1;
        unsigned FBR: 1;
        unsigned OAR2SEL: 1;
        unsigned QCMDST: 1;
        unsigned QCMDRW: 1;
        unsigned : 24;
        unsigned ACTDMATX: 1;
        unsigned ACTDMARX: 1;
    }) CSR;

    // I2C Slave Data
    REGMAP_32(, {
        unsigned DATA: 8;
    }) DR;

    // I2C Slave Interrupt Mask
    REGMAP_32(, {
        unsigned DATA: 1;
        unsigned START: 1;
        unsigned STOP: 1;
        unsigned DMARX: 1;
        unsigned DMATX: 1;
        unsigned TX: 1;
        unsigned RX: 1;
        unsigned TXFE: 1;
        unsigned RXFF: 1;
    }) IMR;

    // I2C Slave Raw Interrupt Status
    REGMAP_32(, {
        unsigned DATA: 1;
        unsigned START: 1;
        unsigned STOP: 1;
        unsigned DMARX: 1;
        unsigned DMATX: 1;
        unsigned TX: 1;
        unsigned RX: 1;
        unsigned TXFE: 1;
        unsigned RXFF: 1;
    }) RIS;

    // I2C Slave Masked Interrupt Status
    REGMAP_32(, {
        unsigned DATA: 1;
        unsigned START: 1;
        unsigned STOP: 1;
        unsigned DMARX: 1;
        unsigned DMATX: 1;
        unsigned TX: 1;
        unsigned RX: 1;
        unsigned TXFE: 1;
        unsigned RXFF: 1;
    }) MIS;

    // I2C Slave Interrupt Clear
    REGMAP_32(, {
        unsigned DATA: 1;
        unsigned START: 1;
        unsigned STOP: 1;
        unsigned DMARX: 1;
        unsigned DMATX: 1;
        unsigned TX: 1;
        unsigned RX: 1;
        unsigned TXFE: 1;
        unsigned RXFF: 1;
    }) ICR;

    // I2C Slave Own Address 2
    REGMAP_32(, {
        unsigned OAR2: 7;
        unsigned OAR2EN: 1;
    }) OAR2;

    // I2C Slave ACK Control
    REGMAP_32(, {
        unsigned ACKOEN: 1;
        unsigned ACKOVAL: 1;
    }) ACKCTL;

};
_Static_assert(sizeof(struct I2C_SLAVE_MAP) == 36, "I2C_SLAVE_MAP must be 36 bytes");

PAGE_MAP(I2C_MAP, {
    // offset 0x000
    // I2C Master Registers
    struct I2C_MASTER_MAP MASTER;

    char _reserved_0[0x7C8];

    // offset 0x800
    // I2C Slave Registers
    struct I2C_SLAVE_MAP SLAVE;

    char _reserved_1[0x6DC];

    // offset 0xF00
    // I2C FIFO Data
    REGMAP_32(, {
        unsigned DATA: 8;
    }) FIFODATA;

    // offset 0xF04
    // I2C FIFO Control
    REGMAP_32(, {
        unsigned TXTRIG: 3;
        unsigned : 10;
        unsigned DMATXENA: 1;
        unsigned TXFLUSH: 1;
        unsigned TXASGNMT: 1;
        unsigned RXTRIG: 3;
        unsigned : 10;
        unsigned DMARXENA: 1;
        unsigned RXFLUSH: 1;
        unsigned RXASGNMT: 1;
    }) FIFOCTL;

    // offset 0xF08
    // I2C FIFO Status
    REGMAP_32(, {
        unsigned TXFE: 1;
        unsigned TXFF: 1;
        unsigned TXBLWTRIG: 1;
        unsigned : 13;
        unsigned RXFE: 1;
        unsigned RXFF: 1;
        unsigned RXBLWTRIG: 1;
    }) FIFOSTATUS;

    char _reserved_2[0x0B4];

    // offset 0xFC0
    // I2C Peripheral Properties
    REGMAP_32(, {
        unsigned HS: 1;
    }) PP;

    // offset 0xFC4
    // I2C Peripheral Configuration
    REGMAP_32(, {
        unsigned HS: 1;
    }) PC;

    char _reserved_3[0x038];
})


#define I2C0    (*(volatile struct I2C_MAP *) 0x40020000)
#define I2C1    (*(volatile struct I2C_MAP *) 0x40021000)
#define I2C2    (*(volatile struct I2C_MAP *) 0x40022000)
#define I2C3    (*(volatile struct I2C_MAP *) 0x40023000)
#define I2C4    (*(volatile struct I2C_MAP *) 0x400C0000)
#define I2C5    (*(volatile struct I2C_MAP *) 0x400C1000)
#define I2C6    (*(volatile struct I2C_MAP *) 0x400C2000)
#define I2C7    (*(volatile struct I2C_MAP *) 0x400C3000)
#define I2C8    (*(volatile struct I2C_MAP *) 0x400B8000)
#define I2C9    (*(volatile struct I2C_MAP *) 0x400B9000)

#endif //TM4C_I2C_H
