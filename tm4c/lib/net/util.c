//
// Created by robert on 5/19/22.
//

#include "../../hw/emac.h"
#include "../format.h"
#include "util.h"


void getMAC(void *mac) {
    EMAC_getMac(&(EMAC0.ADDR0), (uint8_t *) mac);
}

void broadcastMAC(void *_mac) {
    uint8_t *mac = (uint8_t *)_mac;
    mac[0] = 0xFF;
    mac[1] = 0xFF;
    mac[2] = 0xFF;
    mac[3] = 0xFF;
    mac[4] = 0xFF;
    mac[5] = 0xFF;
}

int cmpMac(const void *macA, const void *macB) {
    const int8_t *a = (int8_t *) macA;
    const int8_t *b = (int8_t *) macB;
    if(a[0] != b[0]) return a[0] - b[0];
    if(a[1] != b[1]) return a[1] - b[1];
    if(a[2] != b[2]) return a[2] - b[2];
    if(a[3] != b[3]) return a[3] - b[3];
    if(a[4] != b[4]) return a[4] - b[4];
    if(a[5] != b[5]) return a[5] - b[5];
    return 0;
}

int isNullMAC(const void *_mac) {
    const uint8_t *mac = (uint8_t *)_mac;
    if(mac[0] != 0) return 0;
    if(mac[1] != 0) return 0;
    if(mac[2] != 0) return 0;
    if(mac[3] != 0) return 0;
    if(mac[4] != 0) return 0;
    if(mac[5] != 0) return 0;
    return 1;
}

int isMyMAC(const void *mac) {
    uint8_t myMac[6];
    getMAC(myMac);
    return cmpMac(mac, myMac);
}

void copyMAC(void *_dst, const void *_src) {
    uint8_t *dst = (uint8_t *)_dst;
    const uint8_t *src = (uint8_t *)_src;
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

char* macToStr(const void *_mac, char *str) {
    const uint8_t *mac = (uint8_t *) _mac;
    for(int i = 0; i < 6; i++) {
        str += toHex(mac[i], 2, '0', (char *) str);
        *(str++) = ':';
    }
    str[-1] = 0;
    return (char *) (str - 1);
}
