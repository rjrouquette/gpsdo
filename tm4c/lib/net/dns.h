//
// Created by robert on 12/4/22.
//

#ifndef GPSDO_DNS_H
#define GPSDO_DNS_H

#include <stdint.h>

typedef void (*CallbackDNS)(void *ref, uint32_t remoteAddress);

void DNS_init();
void DNS_updateMAC();
int DNS_lookup(const char *hostname, CallbackDNS callback, void *ref);

#endif //GPSDO_DNS_H
