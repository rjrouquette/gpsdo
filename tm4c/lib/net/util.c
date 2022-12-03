//
// Created by robert on 5/19/22.
//

#include "util.h"
#include "../../hw/emac.h"
#include "../format.h"

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

int isNullMAC(const volatile void *_mac) {
    uint8_t *mac = (uint8_t *)_mac;
    if(mac[0] != 0) return 0;
    if(mac[1] != 0) return 0;
    if(mac[2] != 0) return 0;
    if(mac[3] != 0) return 0;
    if(mac[4] != 0) return 0;
    if(mac[5] != 0) return 0;
    return 1;
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

char* addrToStr(uint32_t addr, char *str) {
    str += toBase((addr >> 0u) & 0xFFu, 10, str);
    *(str++) = '.';
    str += toBase((addr >> 8u) & 0xFFu, 10, str);
    *(str++) = '.';
    str += toBase((addr >> 16u) & 0xFFu, 10, str);
    *(str++) = '.';
    str += toBase((addr >> 24u) & 0xFFu, 10, str);
    *str = 0;
    return str;
}

char* macToStr(uint8_t *mac, char *str) {
    for(int i = 0; i < 6; i++) {
        str += toHex(mac[i], 2, '0', str);
        *(str++) = ':';
    }
    str[-1] = 0;
    return str - 1;
}
