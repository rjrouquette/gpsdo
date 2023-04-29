//
// Created by robert on 5/19/22.
//

#ifndef GPSDO_UTIL_H
#define GPSDO_UTIL_H

#include <stdint.h>

void getMAC(volatile void *mac);
void broadcastMAC(volatile void *mac);

int isNullMAC(const volatile void *mac);
void copyMAC(volatile void *dst, volatile const void *src);

char* addrToStr(uint32_t addr, char *str);
char* macToStr(const volatile uint8_t *mac, volatile char *str);

#endif //GPSDO_UTIL_H
