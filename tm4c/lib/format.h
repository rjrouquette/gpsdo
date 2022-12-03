//
// Created by robert on 4/17/22.
//

#ifndef GPSDO_FORMAT_H
#define GPSDO_FORMAT_H

#include <stdint.h>

char* append(char *dst, const char *src);

int padCopy(int width, char pad,  char *dst, const char *src, int len);

int toBin(uint32_t value, int width, char padding, char *origin);
int toOct(uint32_t value, int width, char padding, char *origin);
int toDec(uint32_t value, int width, char padding, char *origin);
int toHex(uint32_t value, int width, char padding, char *origin);
int toHexLower(uint32_t value, int width, char padding, char *origin);

int toTemp(int16_t value, char *origin);

int toHMS(uint32_t value, char *origin);

int fmtFloat(float value, int width, int digits, char *origin);

#endif //GPSDO_FORMAT_H
