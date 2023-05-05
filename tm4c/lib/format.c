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



#define LUTPOW10_BASE (46)
const static float lutPow10[86] = {
        0, 1e-45f, 1e-44f, 1e-43f, 1e-42f, 1e-41f,
        1e-40f, 1e-39f, 1e-38f, 1e-37f, 1e-36f, 1e-35f, 1e-34f, 1e-33f, 1e-32f, 1e-31f,
        1e-30f, 1e-29f, 1e-28f, 1e-27f, 1e-26f, 1e-25f, 1e-24f, 1e-23f, 1e-22f, 1e-21f,
        1e-20f, 1e-19f, 1e-18f, 1e-17f, 1e-16f, 1e-15f, 1e-14f, 1e-13f, 1e-12f, 1e-11f,
        1e-10f, 1e-09f, 1e-08f, 1e-07f, 1e-06f, 1e-05f, 1e-04f, 1e-03f, 1e-02f, 1e-01f,
        1e+00f, 1e+01f, 1e+02f, 1e+03f, 1e+04f, 1e+05f, 1e+06f, 1e+07f, 1e+08f, 1e+09f,
        1e+10f, 1e+11f, 1e+12f, 1e+13f, 1e+14f, 1e+15f, 1e+16f, 1e+17f, 1e+18f, 1e+19f,
        1e+20f, 1e+21f, 1e+22f, 1e+23f, 1e+24f, 1e+25f, 1e+26f, 1e+27f, 1e+28f, 1e+29f,
        1e+30f, 1e+31f, 1e+32f, 1e+33f, 1e+34f, 1e+35f, 1e+36f, 1e+37f, 1e+38f, INFINITY
};

static inline uint32_t x10(uint32_t value) {
    value += value << 2;
    return value << 1;
}

static int toSci(char *digits, int exponent, int trim) {
    // always at least two digits
    if(trim > 7) trim = 7;
    // shift digits down
    for(int i = 0; i < 8; i++)
        digits[9-i] = digits[8-i];
    // insert decimal
    digits[1] = '.';
    int len = 10 - trim;
    digits[len++] = 'e';
    if(exponent < 0) {
        exponent = -exponent;
        digits[len++] = '-';
    } else {
        digits[len++] = '+';
    }
    return len + toBase(exponent, 10, digits + len);
}

int fmtFloat(float value, int width, int places, char *origin) {
    int len = 0;
    char result[32];
    // extract sign
    if(signbit(value)) {
        result[len++] = '-';
        value = -value;
    }
    // check for NaN
    if(isnan(value)) {
        result[len++] = 'n';
        result[len++] = 'a';
        result[len++] = 'n';
        return padCopy(width, ' ', origin, result, len);
    }
    // check for infinity
    if(isinf(value)) {
        result[len++] = 'i';
        result[len++] = 'n';
        result[len++] = 'f';
        return padCopy(width, ' ', origin, result, len);
    }
    // check for zero
    if(value == 0) {
        result[len++] = '0';
        if(places < 0)
            places = 1;
        if(places > 0) {
            result[len++] = '.';
            while(places-- > 0)
                result[len++] = '0';
        }
        return padCopy(width, ' ', origin, result, len);
    }

    int exponent = (int) floorf(0.3010299957f * (float) ilogbf(value));
    while(value > lutPow10[exponent + LUTPOW10_BASE + 1])
        ++exponent;
    while(value < lutPow10[exponent + LUTPOW10_BASE - 1])
        --exponent;

    uint32_t mantissa = lroundf(value * lutPow10[LUTPOW10_BASE + 8 - exponent]);
    // exact powers of ten are typically over specified
    if(mantissa >= 1000000000u) {
        mantissa /= 10;
        ++exponent;
    }
    // ensure alignment of mantissa
    if(mantissa < 100000000u) {
        mantissa = x10(mantissa);
    }

    // convert mantissa to decimal digits
    for(int i = 8; i >= 0; i--) {
        result[len+i] = lutBase[mantissa % 10];
        mantissa /= 10;
    }
    // trim trailing zeros
    int rtrim = 8;
    while(rtrim > 0 && result[len+rtrim] == '0')
        --rtrim;
    rtrim = 8 - rtrim;

    // automatic notation
    if(places < 0) {
        // leading decimal
        if(exponent < 0 && (8 - exponent - rtrim) < 12) {
            int shift = 1 - exponent;
            // shift digits
            for(int i = 8; i >= 0; i--) {
                int j = len + i + shift;
                if(j < sizeof(result))
                    result[j] = result[len + i];
            }
            // insert leading zeros
            result[len+0] = '0';
            result[len+1] = '.';
            for(int i = 2; i < shift; i++)
                result[len+i] = '0';
            return padCopy(width, ' ', origin, result, len+9+shift-rtrim);
        }

        // trailing decimal
        if(exponent == 8) {
            len += 9;
            result[len++] = '.';
            result[len++] = '0';
            return padCopy(width, ' ', origin, result, len);
        }

        // scientific notation
        if(exponent < 0 || exponent > 8) {
            len += toSci(result + len, exponent, rtrim);
            return padCopy(width, ' ', origin, result, len);
        }

        // embedded decimal point
        for(int i = 8; i > exponent; i--)
            result[len + i + 1] = result[len + i];
        result[len + exponent + 1] = '.';
        int max = 7 - exponent;
        if(rtrim > max) rtrim = max;
        return padCopy(width, ' ', origin, result, len+10-rtrim);
    }

    // width overflow
    if(width > 0 && exponent > (width - places - 2)) {
        len += toSci(result + len, exponent, rtrim);
        return padCopy(width, ' ', origin, result, len);
    }

    // leading decimal
    if(exponent < 0) {
        int shift = 1 - exponent;
        // shift digits
        for(int i = 8; i >= 0; i--) {
            int j = len + i + shift;
            if(j < sizeof(result))
                result[j] = result[len + i];
        }
        // insert leading zeros
        result[len+0] = '0';
        result[len+1] = '.';
        for(int i = 2; i < shift; i++) {
            int j = len + i;
            if(j < sizeof(result))
                result[len + i] = '0';
        }
        len += 2 + places;
        return padCopy(width, ' ', origin, result, len);
    }

    // shift digits
    for(int i = 8; i > exponent; i--)
        result[len + i + 1] = result[len +i];
    // insert decimal zeros
    int end = len + exponent + 1;
    if(places > 0) {
        result[len + exponent + 1] = '.';
        // pad zeros
        int end = len + exponent + 2 + places;
        len += 10;
        while (len < end)
            result[len++] = '0';
    }
    return padCopy(width, ' ', origin, result, end);
}

uint32_t fromHex(const char *str, int len) {
    uint32_t result = 0;
    for(int i = 0; i < len; i++) {
        if(str[i] == 0) break;
        result <<= 4;
        if(str[i] >= '0' && str[i] <= '9') {
            result += str[i] - '0';
        } else if(str[i] >= 'A' && str[i] <= 'F') {
            result += str[i] - 'A';
        } else if(str[i] >= 'a' && str[i] <= 'a') {
            result += str[i] - 'a';
        }
    }
    return result;
}
