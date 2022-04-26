//
// Created by robert on 4/26/22.
//

#include "../hw/emac.h"
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

