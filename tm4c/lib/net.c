//
// Created by robert on 4/26/22.
//

#include "../hw/crc.h"
#include "../hw/emac.h"
#include "../hw/sys.h"
#include "net.h"

void initPHY() {
    // prevent errant transmissions
    EMAC0.PC.PHYHOLD = 1;
    // enable clock
    RCGCEPHY.EN0 = 1;
    while(!PREPHY.RDY0);
    // enable power
    PCEPHY.EN0 = 1;
    while(!PREPHY.RDY0);
    // enable transmissions
    EMAC0.PC.PHYHOLD = 0;
}

void initMAC() {
    // enable clock
    RCGCEMAC.EN0 = 1;
    while(!PREMAC.RDY0);
    // enable power
    PCEMAC.EN0 = 1;
    while(!PREMAC.RDY0);

    // compute MAC address
    CRC.CTRL.TYPE = CRC_TYPE_TCP;
    CRC.CTRL.INIT = CRC_INIT_ZERO;
    CRC.DIN = UNIQUEID.WORD[0];
    CRC.DIN = UNIQUEID.WORD[1];
    CRC.DIN = UNIQUEID.WORD[2];
    CRC.DIN = UNIQUEID.WORD[3];
    // store MAC address (54:55:58:XX:XX:XX)
    EMAC0.ADDR0.LO = 0x58000000u | (CRC.SEED & 0xFFFFFFu);
    EMAC0.ADDR0.HI.ADDR = 0x5455u;
}

void NET_init() {
    initMAC();
    initPHY();
}
