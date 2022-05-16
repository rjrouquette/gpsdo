//
// Created by robert on 4/26/22.
//

#include "../hw/crc.h"
#include "../hw/emac.h"
#include "../hw/gpio.h"
#include "../hw/sys.h"
#include "../lib/delay.h"
#include "../lib/format.h"
#include "net.h"

void initPHY() {
    // configure LEDs
    RCGCGPIO.EN_PORTF = 1;
    delay_cycles_4();
    // unlock GPIO config
    PORTF.LOCK = GPIO_LOCK_KEY;
    PORTF.CR = 0x11u;
    // configure pins
    PORTF.DIR = 0x11u;
    PORTF.DR8R = 0x11u;
    PORTF.PCTL.PMC0 = 0x5;
    PORTF.PCTL.PMC4 = 0x5;
    PORTF.AFSEL.ALT0 = 1;
    PORTF.AFSEL.ALT4 = 1;
    PORTF.DEN = 0x11u;
    // lock GPIO config
    PORTF.CR = 0;
    PORTF.LOCK = 0;

    // prevent errant transmissions
    EMAC0.PC.PHYHOLD = 1;
    // enable clock
    RCGCEPHY.EN0 = 1;
    while(!PREPHY.RDY0);
    // enable power
    PCEPHY.EN0 = 1;
    while(!PREPHY.RDY0);

    // complete configuration
    EMAC0.PC.PHYHOLD = 0;
    uint16_t temp = EMAC_MII_Read(&EMAC0, MII_ADDR_EPHYCFG1);
    temp |= 1u << 15u;
    EMAC_MII_Write(&EMAC0, MII_ADDR_EPHYCFG1, temp);
}

void initPPS() {
    // configure PPS
    RCGCGPIO.EN_PORTG = 1;
    delay_cycles_4();
    // unlock GPIO config
    PORTG.LOCK = GPIO_LOCK_KEY;
    PORTG.CR = 0x01u;
    // configure pins
    PORTG.DIR = 0x01u;
    PORTG.DR8R = 0x01u;
    PORTG.PCTL.PMC0 = 0x5;
    PORTG.AFSEL.ALT0 = 1;
    PORTG.DEN = 0x01u;
    // lock GPIO config
    PORTG.CR = 0;
    PORTG.LOCK = 0;

    // configure PTP
    EMAC0.TIMSTCTRL.TSMAST = 1;
    EMAC0.TIMSTCTRL.PTPIPV4 = 1;
    EMAC0.TIMSTCTRL.PTPIPV6 = 1;
    EMAC0.TIMSTCTRL.PTPVER2 = 1;
    EMAC0.TIMSTCTRL.ALLF = 1;
    EMAC0.TIMSTCTRL.DGTLBIN = 1;
    EMAC0.TIMSTCTRL.TSEN = 1;
    // 25MHz = 40ns
    EMAC0.SUBSECINC.SSINC = 40;

    // PTP clock enabled
    EMAC0.CC.PTPCEN = 1;
}

void initMAC() {
    // enable CRC module
    RCGCCCM.EN = 1;
    while(!PRCCM.RDY);
    // enable clock
    RCGCEMAC.EN0 = 1;
    while(!PREMAC.RDY0);
    // enable power
    PCEMAC.EN0 = 1;
    while(!PREMAC.RDY0);
    // set MII clock
    EMAC0.MIIADDR.CR = 1;
    // initialize PHY
    initPHY();
    initPPS();

    // set upper 24 bits of MAC address
    EMAC0.ADDR0.HI.ADDR = 0x5455u;
    EMAC0.ADDR0.LO = 0x58000000u;
    // set lower 24 bits of MAC address
    CRC.CTRL.TYPE = CRC_TYPE_04C11DB7;
    CRC.CTRL.INIT = CRC_INIT_ZERO;
    CRC.DIN = UNIQUEID.WORD[0];
    CRC.DIN = UNIQUEID.WORD[1];
    CRC.DIN = UNIQUEID.WORD[2];
    CRC.DIN = UNIQUEID.WORD[3];
    EMAC0.ADDR0.LO |= CRC.SEED & 0xFFFFFFu;

    EMAC0.CFG.SADDR = EMAC_SADDR_INS0;
    EMAC0.CFG.RE = 1;
    EMAC0.CFG.TE = 1;

    // LED on when high
    EMAC0.CC.POL = 0;
}

void NET_init() {
    initMAC();
}

void NET_getMacAddress(char *strAddr) {
    strAddr += toHex((EMAC0.ADDR0.HI.ADDR >> 8u) & 0xFFu, 2, '0', strAddr);
    *(strAddr++) = ':';
    strAddr += toHex((EMAC0.ADDR0.HI.ADDR >> 0u) & 0xFFu, 2, '0', strAddr);
    *(strAddr++) = ':';
    strAddr += toHex((EMAC0.ADDR0.LO >> 24u) & 0xFFu, 2, '0', strAddr);
    *(strAddr++) = ':';
    strAddr += toHex((EMAC0.ADDR0.LO >> 16u) & 0xFFu, 2, '0', strAddr);
    *(strAddr++) = ':';
    strAddr += toHex((EMAC0.ADDR0.LO >> 8u) & 0xFFu, 2, '0', strAddr);
    *(strAddr++) = ':';
    strAddr += toHex((EMAC0.ADDR0.LO >> 0u) & 0xFFu, 2, '0', strAddr);
    *strAddr = 0;
}
