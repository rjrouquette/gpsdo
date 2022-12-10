//
// Created by robert on 4/17/22.
//

#include <math.h>
#include "format.h"

const char lutBase[16] = "0123456789ABCDEF";

char* append(char *dst, const char *src) {
    for(;;) {
        (*dst) = (*src);
        if((*src) == 0) break;
        ++dst; ++src;
    }
    return dst;
}

char* toHexBytes(char *tail, const uint8_t *bytes, int size) {
    for(int i = 0; i < size; i++) {
        *(tail++) = lutBase[bytes[i] >> 4];
        *(tail++) = lutBase[bytes[i] & 15];
        *(tail++) = ' ';
    }
    return --tail;
}

int padCopy(int width, char pad,  char *dst, const char *src, int len) {
    if(width <= 0) {
        for(int i = 0; i < len; i++)
            dst[i] = src[i];
        return len;
    }
    if(width <= len) {
        int j = len-width;
        for(int i = 0; i < width; i++)
            dst[i] = src[i+j];
        return width;
    }
    int j = width-len;
    for(int i = 0; i < j; i++)
        dst[i] = pad;
    for(int i = 0; i < len; i++)
        dst[i+j] = src[i];
    return width;
}

int toBase(uint32_t value, char base, char *result) {
    char digits[32];
    int cnt = 0;
    while(value > 0) {
        digits[cnt++] = lutBase[value % base];
        value /= base;
    }
    if(cnt < 1) {
        result[0] = lutBase[0];
        return 1;
    }
    for(int i = 0; i < cnt; i++)
        result[cnt - i - 1] = digits[i];
    return cnt;
}

int fmtBase(uint32_t value, char base, int width, char padding, char *origin) {
    char digits[32];
    int cnt = toBase(value, base, digits);
    return padCopy(width, padding, origin, digits, cnt);
}

int toBin(uint32_t value, int width, char padding, char *origin) {
    return fmtBase(value, 2, width, padding, origin);
}

int toOct(uint32_t value, int width, char padding, char *origin) {
    return fmtBase(value, 8, width, padding, origin);
}

int toDec(uint32_t value, int width, char padding, char *origin) {
    return fmtBase(value, 10, width, padding, origin);
}

int toHex(uint32_t value, int width, char padding, char *origin) {
    return fmtBase(value, 16, width, padding, origin);
}

int toTemp(int16_t value, char *origin) {
    fmtFloat(ldexpf((float) value, -8), 7, 2, origin);
    origin[7] = 0xBA;
    origin[8] = 'C';
    return 9;
}

int toHMS(uint32_t value, char *origin) {
    toDec(value % 60, 2, '0', origin + 6);
    value /= 60;
    toDec(value % 60, 2, '0', origin + 3);
    value /= 60;
    toDec(value % 24, 2, '0', origin);
    origin[2] = ':';
    origin[5] = ':';
    return 8;
}

static const float lutFracMult[8] = {
        1e1f, 1e2f, 1e3f, 1e4f, 1e5f, 1e6f, 1e7f, 1e8f
};

int fmtFloat(float value, int width, int digits, char *origin) {
    int len = 0;
    char result[32];
    if(value < 0) {
        value = -value;
        result[len++] = '-';
    }
    if(digits < 1) {
        len += toBase(lroundf(value), 10, result + len);
        return padCopy(width, ' ', origin, result, len);
    }
    // print integer part
    uint32_t ipart = (uint32_t) value;
    value -= (float) ipart;
    len += toBase(ipart, 10, result + len);
    // append decimal point
    result[len++] = '.';
    // restrict digits
    if(digits > 8) digits = 8;
    // format decimal digits
    char tmp[8];
    int dlen = toBase(lroundf(value * lutFracMult[digits-1]), 10, tmp);
    padCopy(digits, '0', result+len, tmp, dlen);
    len += digits;
    return padCopy(width, ' ', origin, result, len);
}
