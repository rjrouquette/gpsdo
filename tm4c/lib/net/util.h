//
// Created by robert on 5/19/22.
//

#ifndef GPSDO_UTIL_H
#define GPSDO_UTIL_H

#include <stdint.h>

void getMAC(volatile void *mac);
void broadcastMAC(volatile void *mac);

void copyMAC(volatile void *dst, volatile const void *src);

#endif //GPSDO_UTIL_H
