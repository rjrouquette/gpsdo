//
// Created by robert on 5/19/22.
//

#include "util.h"
#include "../../hw/emac.h"

void getMAC(volatile void *_mac) {
    uint8_t *mac = (uint8_t *)_mac;
    mac[5] = (EMAC0.ADDR0.HI.ADDR >> 8) & 0xFF;;
    mac[4] = (EMAC0.ADDR0.HI.ADDR >> 0) & 0xFF;;
    mac[3] = (EMAC0.ADDR0.LO >> 24) & 0xFF;
    mac[2] = (EMAC0.ADDR0.LO >> 16) & 0xFF;
    mac[1] = (EMAC0.ADDR0.LO >> 8) & 0xFF;
    mac[0] = (EMAC0.ADDR0.LO >> 0) & 0xFF;
}

void broadcastMAC(volatile void *_mac) {
    uint8_t *mac = (uint8_t *)_mac;
    mac[0] = 0xFF;
    mac[1] = 0xFF;
    mac[2] = 0xFF;
    mac[3] = 0xFF;
    mac[4] = 0xFF;
    mac[5] = 0xFF;
}

void copyMAC(volatile void *_dst, volatile const void *_src) {
    uint8_t *dst = (uint8_t *)_dst;
    uint8_t *src = (uint8_t *)_src;
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
    dst[3] = src[3];
    dst[4] = src[4];
    dst[5] = src[5];
}
