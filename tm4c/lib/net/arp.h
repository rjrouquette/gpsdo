//
// Created by robert on 5/16/22.
//

#ifndef GPSDO_ARP_H
#define GPSDO_ARP_H

#include <stdint.h>

void ARP_poll();
void ARP_process(uint8_t *packet);

#endif //GPSDO_ARP_H
