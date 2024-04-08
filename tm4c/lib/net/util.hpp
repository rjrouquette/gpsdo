//
// Created by robert on 5/19/22.
//

#pragma once

#include <cstdint>

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
 * @param mac
 * @param str
 * @return pointer to end of MAC string
 */
char* macToStr(const void *mac, char *str);

// replicate htons() function
__attribute__((always_inline))
inline uint16_t htons(uint16_t value) {
    return __builtin_bswap16(value);
}

// replicate htonl() function
__attribute__((always_inline))
inline uint32_t htonl(uint32_t value) {
    return __builtin_bswap32(value);
}

// replicate htonll() function
__attribute__((always_inline))
inline uint64_t htonll(uint64_t value) {
    return __builtin_bswap64(value);
}
