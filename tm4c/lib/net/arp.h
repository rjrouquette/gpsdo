//
// Created by robert on 5/16/22.
//

#ifndef GPSDO_ARP_H
#define GPSDO_ARP_H

#include <stdint.h>

#ifndef PACKED
#define PACKED __attribute__((packed))
#endif

#define ARP_OP_REQUEST (0x0100)
#define ARP_OP_REPLY (0x0200)

struct PACKED PAYLOAD_ARP_IP4 {
        uint16_t HTYPE;
        uint16_t PTYPE;
        uint8_t HLEN;
        uint8_t PLEN;
        uint16_t OPER;
        uint8_t SHA[6];
        uint32_t SPA;
        uint8_t THA[6];
        uint32_t TPA;
};
_Static_assert(sizeof(struct PAYLOAD_ARP_IP4) == 28, "PAYLOAD_ARP_IP4 must be 28 bytes");

#define ARP_FRAME_LEN (60)

typedef void (*CallbackARP)(void *ref, uint32_t remoteAddress, uint8_t *macAddress);

void ARP_announce();
void ARP_run();
void ARP_process(uint8_t *frame, int flen);
int ARP_request(uint32_t remoteAddress, CallbackARP callback, volatile void *ref);

#endif //GPSDO_ARP_H
