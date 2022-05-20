//
// Created by robert on 5/16/22.
//

#ifndef GPSDO_ARP_H
#define GPSDO_ARP_H

#include <stdint.h>

#ifndef PACKED
#define PACKED __attribute__((packed))
#endif

#define ARP_OP_REQUEST (1)
#define ARP_OP_REPLY (2)

struct PACKED PAYLOAD_ARP_IP4 {
        uint8_t HTYPE[2];
        uint8_t PTYPE[2];
        uint8_t HLEN;
        uint8_t PLEN;
        uint8_t OPER[2];
        uint8_t SHA[6];
        uint8_t SPA[4];
        uint8_t THA[6];
        uint8_t TPA[4];
};
_Static_assert(sizeof(struct PAYLOAD_ARP_IP4) == 28, "PAYLOAD_ARP_IP4 must be 28 bytes");

#define ARP_FRAME_LEN (64)

typedef void (*CallbackARP)(uint32_t ipAddress, uint8_t *macAddress);

void ARP_poll();
void ARP_process(uint8_t *frame, int flen);
int ARP_request(uint32_t remoteAddress, CallbackARP callback);

#endif //GPSDO_ARP_H
