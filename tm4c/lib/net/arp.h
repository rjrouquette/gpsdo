//
// Created by robert on 5/16/22.
//

#ifndef GPSDO_ARP_H
#define GPSDO_ARP_H

#include <stdint.h>

typedef void (*CallbackARP)(uint32_t ipAddress, uint8_t *macAddress);

void ARP_poll();
void ARP_process(uint8_t *packet);
int ARP_request(uint32_t ipAddress, CallbackARP callback);

#endif //GPSDO_ARP_H
