//
// Created by robert on 5/16/22.
//

#ifndef GPSDO_IP_H
#define GPSDO_IP_H

#include <stdint.h>

#ifndef PACKED
#define PACKED __attribute__((packed))
#endif

#define IP_PROTO_ICMP   (1)
#define IP_PROTO_TCP    (6)
#define IP_PROTO_UDP    (17)

struct PACKED HEADER_IPv4 {
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
};
_Static_assert(sizeof(struct HEADER_IPv4) == 20, "HEADER_IPv4 must be 20 bytes");

extern volatile uint32_t ipAddress;
extern volatile uint32_t ipSubnet;
extern volatile uint32_t ipGateway;
extern volatile uint32_t ipDNS;

void IPv4_process(uint8_t *frame, int flen);
void IPv4_init(uint8_t *frame);
void IPv4_finalize(uint8_t *frame, int flen);

uint16_t RFC1071_checksum(volatile const void *buffer, int len);

#endif //GPSDO_IP_H
