//
// Created by robert on 5/16/22.
//

#pragma once

#include <cstdint>

#define ARP_MAX_AGE (64) // refresh old MAC addresses

typedef void (*CallbackARP)(void *ref, uint32_t remoteAddress, const uint8_t *macAddress);

extern uint8_t macRouter[6];

void ARP_init();

void ARP_process(uint8_t *frame, int size);
int ARP_request(uint32_t remoteAddress, CallbackARP callback, void *ref);
void ARP_refreshRouter();
