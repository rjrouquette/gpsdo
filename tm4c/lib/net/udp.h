//
// Created by robert on 5/20/22.
//

#ifndef GPSDO_UDP_H
#define GPSDO_UDP_H

#include <stdint.h>

#ifndef PACKED
#define PACKED __attribute__((packed))
#endif

struct PACKED HEADER_UDP {
    uint8_t portSrc[2];
    uint8_t portDst[2];
    uint8_t length[2];
    uint8_t chksum[2];
};
_Static_assert(sizeof(struct HEADER_UDP) == 8, "HEADER_UDP must be 8 bytes");

typedef void (*CallbackUDP)(uint8_t *frame, int flen);

void UDP_process(uint8_t *frame, int flen);
void UDP_finalize(uint8_t *frame, int flen);

int UDP_register(uint16_t port, CallbackUDP callback);
int UDP_deregister(uint16_t port);

#endif //GPSDO_UDP_H
