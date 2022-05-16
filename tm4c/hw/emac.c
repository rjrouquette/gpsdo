//
// Created by robert on 5/15/22.
//

#include "emac.h"

void EMAC_MII_Write(volatile struct EMAC_MAP *emac, uint8_t address, uint16_t value) {
    // wait for MII to be ready
    while(emac->MIIADDR.MIB);
    emac->MIIDATA.DATA = value;
    emac->MIIADDR.MII = address;
    emac->MIIADDR.MIW = 1;
    emac->MIIADDR.MIB = 1;
}

uint16_t EMAC_MII_Read(volatile struct EMAC_MAP *emac, uint8_t address) {
    // wait for MII to be ready
    while(emac->MIIADDR.MIB);
    emac->MIIADDR.MII = address;
    emac->MIIADDR.MIW = 0;
    emac->MIIADDR.MIB = 1;
    // wait for MII to be ready
    while(emac->MIIADDR.MIB);
    // return value
    return emac->MIIDATA.DATA;
}
