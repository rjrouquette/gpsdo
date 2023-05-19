//
// Created by robert on 5/19/22.
//

#ifndef GPSDO_ICMP_H
#define GPSDO_ICMP_H

#include <stdint.h>

#ifndef PACKED
#define PACKED __attribute__((packed))
#endif

typedef struct PACKED HEADER_ICMP4 {
    uint8_t type;
    uint8_t code;
    uint8_t chksum[2];
    uint8_t misc[4];
} HEADER_ICMP4;
_Static_assert(sizeof(struct HEADER_ICMP4) == 8, "HEADER_ICMP4 must be 8 bytes");


void ICMP_process(uint8_t *frame, int flen);

#endif //GPSDO_ICMP_H
