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
    uint16_t portSrc;
    uint16_t portDst;
    uint16_t length;
    uint16_t chksum;
};
#ifdef __cplusplus
static_assert(sizeof(struct HEADER_UDP) == 8, "HEADER_UDP must be 8 bytes");
#else
_Static_assert(sizeof(struct HEADER_UDP) == 8, "HEADER_UDP must be 8 bytes");
#endif

#define UDP_DATA_OFFSET (14+20+8)

typedef void (*CallbackUDP)(uint8_t *frame, int flen);

void UDP_process(uint8_t *frame, int flen);
void UDP_finalize(uint8_t *frame, int flen);
void UDP_returnToSender(uint8_t *frame, uint32_t ipAddr, uint16_t port);

int UDP_register(uint16_t port, CallbackUDP callback);
int UDP_deregister(uint16_t port);

#endif //GPSDO_UDP_H
