//
// Created by robert on 4/17/22.
//

#include "format.h"

const char lut_base[16] = "0123456789ABCDEF";

int toBase(unsigned value, char base, int width, char padding, char *origin) {
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

int toBin(unsigned value, int width, char padding, char *origin) {
    return toBase(value, 2, width, padding, origin);
}

int toOct(unsigned value, int width, char padding, char *origin) {
    return toBase(value, 8, width, padding, origin);
}

int toDec(unsigned value, int width, char padding, char *origin) {
    return toBase(value, 10, width, padding, origin);
}

int toHex(unsigned value, int width, char padding, char *origin) {
    return toBase(value, 16, width, padding, origin);
}
