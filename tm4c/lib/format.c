//
// Created by robert on 4/17/22.
//

#include "format.h"

const char lut_base[16] = "0123456789ABCDEF";

int toBase(unsigned value, char base, int width, char padding, char *origin) {
    char digits[16];
    int cnt = 0;
    while(value) {
        digits[cnt++] = lut_base[value % base];
        value /= base;
    }
    if(cnt == 0) {
        digits[0] = 0;
        cnt = 1;
    }

    if(width <= 0) {
        width = cnt;
        origin += width - 1;
    }

    for (int offset = 0; offset < width; offset++) {
        if (offset < cnt)
            origin[-offset] = digits[offset];
        else
            origin[-offset] = padding;
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
