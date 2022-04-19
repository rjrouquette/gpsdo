//
// Created by robert on 4/17/22.
//

#include "format.h"

const char lut_base[16] = "0123456789ABCDEF";

int toBase(uint32_t value, char base, int width, char padding, char *origin) {
    char digits[16];
    int cnt = 0;
    while(value > 0) {
        digits[cnt++] = lut_base[value % base];
        value /= base;
    }
    if(cnt < 1) {
        digits[0] = lut_base[0];
        cnt = 1;
    }

    if(width <= 0)
        width = cnt;
    origin += width - 1;

    int i = 0;
    while(i < cnt) {
        origin[-i] = digits[i];
        ++i;
    }
    while(i < width) {
        origin[-i] = padding;
        ++i;
    }
    return width;
}

int toBin(uint32_t value, int width, char padding, char *origin) {
    return toBase(value, 2, width, padding, origin);
}

int toOct(uint32_t value, int width, char padding, char *origin) {
    return toBase(value, 8, width, padding, origin);
}

int toDec(uint32_t value, int width, char padding, char *origin) {
    return toBase(value, 10, width, padding, origin);
}

int toHex(uint32_t value, int width, char padding, char *origin) {
    return toBase(value, 16, width, padding, origin);
}

int toTemp(int16_t value, char *origin) {
    if(value < 0) {
        origin[0] = '-';
        value = (int16_t) -value;
    } else {
        origin[0] = '+';
    }
    toDec(value >> 8u, 3, '0', origin + 1);
    toDec((10 * (value & 0xFFu)) >> 8u, 1, '0', origin + 5);
    origin[4] = '.';
    origin[6] = 0xBA;
    origin[7] = 'C';
    return 8;
}
