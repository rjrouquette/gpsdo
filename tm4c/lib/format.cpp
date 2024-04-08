//
// Created by robert on 4/17/22.
//

#include "format.h"

#include <cmath>

static constexpr char lutBase[] = "0123456789ABCDEF";

char* append(char *dst, const char *src) {
    for (;;) {
        *dst = *src;
        if (*src == 0)
            break;
        ++dst;
        ++src;
    }
    return dst;
}

char* toHexBytes(char *tail, const uint8_t *bytes, const int size) {
    for (int i = 0; i < size; i++) {
        *tail++ = lutBase[bytes[i] >> 4];
        *tail++ = lutBase[bytes[i] & 15];
        *tail++ = ' ';
    }
    return --tail;
}

int padCopy(const int width, const char pad, char *dst, const char *src, const int len) {
    if (width <= 0) {
        for (int i = 0; i < len; i++)
            dst[i] = src[i];
        return len;
    }
    if (width <= len) {
        const int j = len - width;
        for (int i = 0; i < width; i++)
            dst[i] = src[i + j];
        return width;
    }
    const int j = width - len;
    for (int i = 0; i < j; i++)
        dst[i] = pad;
    for (int i = 0; i < len; i++)
        dst[i + j] = src[i];
    return width;
}

int toBase(uint32_t value, const char base, char *result) {
    char digits[32];
    int cnt = 0;
    while (value > 0) {
        digits[cnt++] = lutBase[value % base];
        value /= base;
    }
    if (cnt < 1) {
        result[0] = lutBase[0];
        return 1;
    }
    for (int i = 0; i < cnt; i++)
        result[cnt - i - 1] = digits[i];
    return cnt;
}

int fmtBase(const uint32_t value, const char base, const int width, const char padding, char *origin) {
    char digits[32];
    const int cnt = toBase(value, base, digits);
    return padCopy(width, padding, origin, digits, cnt);
}

int toBin(const uint32_t value, const int width, const char padding, char *origin) {
    return fmtBase(value, 2, width, padding, origin);
}

int toOct(const uint32_t value, const int width, const char padding, char *origin) {
    return fmtBase(value, 8, width, padding, origin);
}

int toDec(const uint32_t value, const int width, const char padding, char *origin) {
    return fmtBase(value, 10, width, padding, origin);
}

int toHex(const uint32_t value, const int width, const char padding, char *origin) {
    return fmtBase(value, 16, width, padding, origin);
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


static constexpr float lutRawPow10[86] = {
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
static constexpr const float *lutPow10 = lutRawPow10 + 46;

static int toSci(char *digits, int exponent, int trim) {
    // always at least two digits
    if (trim > 7)
        trim = 7;
    // shift digits down
    for (int i = 0; i < 8; i++)
        digits[9 - i] = digits[8 - i];
    // insert decimal
    digits[1] = '.';
    int len = 10 - trim;
    digits[len++] = 'e';
    if (exponent < 0) {
        exponent = -exponent;
        digits[len++] = '-';
    }
    else {
        digits[len++] = '+';
    }
    return len + toBase(exponent, 10, digits + len);
}

int fmtFloat(float value, const int width, int places, char *origin) {
    int len = 0;
    char result[32];
    // extract sign
    if (std::signbit(value)) {
        result[len++] = '-';
        value = -value;
    }
    // check for NaN
    if (std::isnan(value)) {
        result[len++] = 'n';
        result[len++] = 'a';
        result[len++] = 'n';
        return padCopy(width, ' ', origin, result, len);
    }
    // check for infinity
    if (std::isinf(value)) {
        result[len++] = 'i';
        result[len++] = 'n';
        result[len++] = 'f';
        return padCopy(width, ' ', origin, result, len);
    }
    // check for zero
    if (value == 0) {
        result[len++] = '0';
        if (places < 0)
            places = 1;
        if (places > 0) {
            result[len++] = '.';
            while (places-- > 0)
                result[len++] = '0';
        }
        return padCopy(width, ' ', origin, result, len);
    }

    int exponent = static_cast<int>(floorf(0.3010299957f * static_cast<float>(ilogbf(value))));
    while (value > lutPow10[exponent + 1])
        ++exponent;
    while (value < lutPow10[exponent - 1])
        --exponent;

    // assemble mantissa
    uint32_t mantissa;
    // cap exponent
    if (exponent > 38)
        exponent = 38;
    // tiny values need to be scaled twice to avoid overflow
    if (exponent < -30) {
        value *= lutPow10[30];
        mantissa = lroundf(value * lutPow10[-22 - exponent]);
    }
    else {
        mantissa = lroundf(value * lutPow10[8 - exponent]);
    }

    // exact powers of ten are typically over specified
    if (mantissa >= 1000000000u) {
        mantissa /= 10;
        ++exponent;
    }
    // ensure alignment of mantissa
    if (mantissa < 100000000u)
        mantissa *= 10;

    // convert mantissa to decimal digits
    for (int i = 8; i >= 0; i--) {
        result[len + i] = lutBase[mantissa % 10];
        mantissa /= 10;
    }
    // trim trailing zeros
    int rtrim = 8;
    while (rtrim > 0 && result[len + rtrim] == '0')
        --rtrim;
    rtrim = 8 - rtrim;

    // automatic notation
    if (places < 0) {
        // leading decimal
        if (exponent < 0 && 8 - exponent - rtrim < 12) {
            const int shift = 1 - exponent;
            // shift digits
            for (int i = 8; i >= 0; i--) {
                const int j = len + i + shift;
                if (j < static_cast<int>(sizeof(result)))
                    result[j] = result[len + i];
            }
            // insert leading zeros
            result[len + 0] = '0';
            result[len + 1] = '.';
            for (int i = 2; i < shift; i++)
                result[len + i] = '0';
            return padCopy(width, ' ', origin, result, len + 9 + shift - rtrim);
        }

        // trailing decimal
        if (exponent == 8) {
            len += 9;
            result[len++] = '.';
            result[len++] = '0';
            return padCopy(width, ' ', origin, result, len);
        }

        // scientific notation
        if (exponent < 0 || exponent > 8) {
            len += toSci(result + len, exponent, rtrim);
            return padCopy(width, ' ', origin, result, len);
        }

        // embedded decimal point
        for (int i = 8; i > exponent; i--)
            result[len + i + 1] = result[len + i];
        result[len + exponent + 1] = '.';
        const int max = 7 - exponent;
        if (rtrim > max)
            rtrim = max;
        return padCopy(width, ' ', origin, result, len + 10 - rtrim);
    }

    // width overflow
    if (width > 0 && exponent > width - places - 2) {
        len += toSci(result + len, exponent, rtrim);
        return padCopy(width, ' ', origin, result, len);
    }

    // leading decimal
    if (exponent < 0) {
        const int shift = 1 - exponent;
        // shift digits
        for (int i = 8; i >= 0; i--) {
            const int j = len + i + shift;
            if (j < static_cast<int>(sizeof(result)))
                result[j] = result[len + i];
        }
        // insert leading zeros
        result[len + 0] = '0';
        if (places > 0) {
            result[len + 1] = '.';
            for (int i = 2; i < shift; i++) {
                const int j = len + i;
                if (j < static_cast<int>(sizeof(result)))
                    result[len + i] = '0';
            }
            len += 2 + places;
        }
        else {
            ++len;
        }
        return padCopy(width, ' ', origin, result, len);
    }

    // shift digits
    for (int i = 8; i > exponent; i--)
        result[len + i + 1] = result[len + i];
    // insert decimal zeros
    int end = len + exponent + 1;
    if (places > 0) {
        result[len + exponent + 1] = '.';
        // pad zeros
        end = len + exponent + 2 + places;
        len += 10;
        while (len < end)
            result[len++] = '0';
    }
    return padCopy(width, ' ', origin, result, end);
}

uint32_t fromHex(const char *str, const int len) {
    uint32_t result = 0;
    for (int i = 0; i < len; i++) {
        if (str[i] == 0)
            break;
        result <<= 4;
        if (str[i] >= '0' && str[i] <= '9') {
            result += str[i] - '0';
        }
        else if (str[i] >= 'A' && str[i] <= 'F') {
            result += str[i] - 'A';
        }
        else if (str[i] >= 'a' && str[i] <= 'a') {
            result += str[i] - 'a';
        }
    }
    return result;
}
