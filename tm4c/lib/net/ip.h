//
// Created by robert on 5/16/22.
//

#ifndef GPSDO_IP_H
#define GPSDO_IP_H

#ifdef __cplusplus
extern "C" {
#else
#define static_assert _Static_assert
#endif

#include <stdint.h>

#ifndef PACKED
#define PACKED __attribute__((packed))
#endif

#define IP_PROTO_ICMP   (1)
#define IP_PROTO_TCP    (6)
#define IP_PROTO_UDP    (17)

typedef struct PACKED HEADER_IP4 {
    union PACKED {
        struct PACKED {
            unsigned IHL: 4;
            unsigned VER: 4;
            unsigned ECN: 2;
            unsigned DSCP: 6;
        };
        uint16_t bits;
    } head;
    uint16_t len;
    uint16_t id;
    uint8_t flags;
    uint8_t frag;
    uint8_t ttl;
    uint8_t proto;
    uint16_t chksum;
    uint32_t src;
    uint32_t dst;
} HEADER_IP4;
static_assert(sizeof(struct HEADER_IP4) == 20, "HEADER_IP4 must be 20 bytes");

extern volatile uint32_t ipBroadcast;
extern volatile uint32_t ipAddress;
extern volatile uint32_t ipSubnet;
extern volatile uint32_t ipRouter;
extern volatile uint32_t ipDNS;

/**
 * Check IPv4 subnet co-membership
 * @param subnetMask IPv4 subnet mask
 * @param addrA IPv4 address
 * @param addrB IPv4 address
 * @return non-zero if addresses fall in different subnets
 */
__attribute__((always_inline))
inline uint32_t IPv4_testSubnet(uint32_t subnetMask, uint32_t addrA, uint32_t addrB) {
    return (addrA ^ addrB) & subnetMask;
}

void IPv4_process(uint8_t *frame, int flen);
void IPv4_init(uint8_t *frame);
void IPv4_finalize(uint8_t *frame, int flen);
void IPv4_setMulticast(uint8_t *frame, uint32_t groupAddress);

/**
 * Calculate MAC address for IPv4 multicast group
 * @param mac resulting MAC address (network byte-order)
 * @param groupAddress IPv4 group address (network byte-order)
 */
void IPv4_macMulticast(uint8_t *mac, uint32_t groupAddress);

uint16_t RFC1071_checksum(volatile const void *buffer, int len);

#ifdef __cplusplus
}
#endif

#endif //GPSDO_IP_H
