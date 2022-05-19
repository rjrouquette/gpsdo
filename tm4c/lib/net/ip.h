//
// Created by robert on 5/16/22.
//

#ifndef GPSDO_IP_H
#define GPSDO_IP_H

#include <stdint.h>

extern volatile uint32_t ipAddress;
extern volatile uint32_t ipSubnet;
extern volatile uint32_t ipGateway;
extern volatile uint32_t ipDNS;

#endif //GPSDO_IP_H
