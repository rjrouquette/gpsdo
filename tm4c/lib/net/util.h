//
// Created by robert on 5/19/22.
//

#ifndef GPSDO_UTIL_H
#define GPSDO_UTIL_H

#include <memory.h>
#include <stdint.h>

/**
 * Get the primary MAC address of the NIC
 * @param mac
 */
void getMAC(void *mac);

/**
 * Set to universal broadcast MAC address (FF:FF:FF:FF:FF:FF)
 * @param mac
 */
void broadcastMAC(void *mac);

/**
 * Compare two MAC addresses
 * @param macA
 * @param macB
 * @return comparison result (zero for equality)
 */
int cmpMac(const void *macA, const void *macB);

/**
 * Determine if MAC address is NULL (00:00:00:00:00:00)
 * @param mac
 * @return zero if non-null, 1 otherwise
 */
int isNullMAC(const void *mac);

/**
 * Determine if MAC address is my address
 * @param mac
 * @return zero if MAC matches NIC
 */
int isMyMAC(const void *mac);

/**
 * Copy MAC address
 * @param dst
 * @param src
 */
void copyMAC(void *dst, const void *src);

/**
 * Get string representation of IPv4 address
 * @param addr
 * @param str
 * @return pointer to end of IPv4 string
 */
char* addrToStr(uint32_t addr, char *str);

/**
 * Get string representation of MAC address
 * @param addr
 * @param str
 * @return pointer to end of MAC string
 */
char* macToStr(const void *mac, char *str);

#endif //GPSDO_UTIL_H
