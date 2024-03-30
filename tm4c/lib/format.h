//
// Created by robert on 4/17/22.
//

#ifndef GPSDO_FORMAT_H
#define GPSDO_FORMAT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

char* append(char *dst, const char *src);
char* toHexBytes(char *tail, const uint8_t *bytes, int size);

int padCopy(int width, char pad,  char *dst, const char *src, int len);

int toBase(uint32_t value, char base, char *result);

int toBin(uint32_t value, int width, char padding, char *origin);
int toOct(uint32_t value, int width, char padding, char *origin);
int toDec(uint32_t value, int width, char padding, char *origin);
int toHex(uint32_t value, int width, char padding, char *origin);

int toHMS(uint32_t value, char *origin);

int fmtFloat(float value, int width, int places, char *origin);

uint32_t fromHex(const char *str, int len);

#ifdef __cplusplus
}
#endif

#endif //GPSDO_FORMAT_H
