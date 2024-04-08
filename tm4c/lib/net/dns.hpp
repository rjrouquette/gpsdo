//
// Created by robert on 12/4/22.
//

#pragma once

#include <cstdint>

typedef void (*CallbackDNS)(void *ref, uint32_t remoteAddress);

void DNS_init();
void DNS_updateMAC();
int DNS_lookup(const char *hostname, CallbackDNS callback, volatile void *ref);
