//
// Created by robert on 5/19/22.
//

#include "util.hpp"

#include "../format.hpp"
#include "../../hw/emac.h"


void getMAC(void *mac) {
    EMAC_getMac(&EMAC0.ADDR0, static_cast<uint8_t*>(mac));
}

void broadcastMAC(void *mac) {
    const auto _mac = static_cast<uint8_t*>(mac);
    _mac[0] = 0xFF;
    _mac[1] = 0xFF;
    _mac[2] = 0xFF;
    _mac[3] = 0xFF;
    _mac[4] = 0xFF;
    _mac[5] = 0xFF;
}

int cmpMac(const void *macA, const void *macB) {
    const auto a = static_cast<const int8_t*>(macA);
    const auto b = static_cast<const int8_t*>(macB);
    if (a[0] != b[0])
        return a[0] - b[0];
    if (a[1] != b[1])
        return a[1] - b[1];
    if (a[2] != b[2])
        return a[2] - b[2];
    if (a[3] != b[3])
        return a[3] - b[3];
    if (a[4] != b[4])
        return a[4] - b[4];
    if (a[5] != b[5])
        return a[5] - b[5];
    return 0;
}

int isNullMAC(const void *mac) {
    const auto _mac = static_cast<const uint8_t*>(mac);
    if (_mac[0] != 0)
        return 0;
    if (_mac[1] != 0)
        return 0;
    if (_mac[2] != 0)
        return 0;
    if (_mac[3] != 0)
        return 0;
    if (_mac[4] != 0)
        return 0;
    if (_mac[5] != 0)
        return 0;
    return 1;
}

int isMyMAC(const void *mac) {
    uint8_t myMac[6];
    getMAC(myMac);
    return cmpMac(mac, myMac);
}

void copyMAC(void *dst, const void *src) {
    const auto _dst = static_cast<uint8_t*>(dst);
    const auto _src = static_cast<const uint8_t*>(src);
    _dst[0] = _src[0];
    _dst[1] = _src[1];
    _dst[2] = _src[2];
    _dst[3] = _src[3];
    _dst[4] = _src[4];
    _dst[5] = _src[5];
}

char* addrToStr(const uint32_t addr, char *str) {
    str += toBase((addr >> 0u) & 0xFFu, 10, str);
    *str++ = '.';
    str += toBase((addr >> 8u) & 0xFFu, 10, str);
    *str++ = '.';
    str += toBase((addr >> 16u) & 0xFFu, 10, str);
    *str++ = '.';
    str += toBase((addr >> 24u) & 0xFFu, 10, str);
    *str = 0;
    return str;
}

char* macToStr(const void *mac, char *str) {
    const auto _mac = static_cast<const uint8_t*>(mac);
    for (int i = 0; i < 6; i++) {
        str += toHex(_mac[i], 2, '0', str);
        *str++ = ':';
    }
    *--str = 0;
    return str;
}
