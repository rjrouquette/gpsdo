//
// Created by robert on 5/19/22.
//

#ifndef GPSDO_UTIL_H
#define GPSDO_UTIL_H

#include <stdint.h>

void getMAC(void *mac);
void broadcastMAC(void *mac);

int isNullMAC(const void *mac);
void copyMAC(void *dst, const void *src);

char* addrToStr(uint32_t addr, char *str);
char* macToStr(const uint8_t *mac, char *str);

#endif //GPSDO_UTIL_H
