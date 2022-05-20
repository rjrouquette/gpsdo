//
// Created by robert on 5/20/22.
//

#include <memory.h>

#include "eth.h"
#include "ip.h"
#include "udp.h"

#define MAX_ENTRIES (8)
static volatile uint16_t registryPort[MAX_ENTRIES];
static volatile CallbackUDP registryCallback[MAX_ENTRIES];

void UDP_process(uint8_t *frame, int flen) {
    // discard malformed packets
    if(flen < 42) return;
    // map headers
    struct FRAME_ETH *headerEth = (struct FRAME_ETH *) frame;
    struct HEADER_IPv4 *headerIPv4 = (struct HEADER_IPv4 *) (headerEth + 1);
    struct HEADER_UDP *headerUDP = (struct HEADER_UDP *) (headerIPv4 + 1);
    // invoke handler
    uint16_t port;
    memcpy(&port, headerUDP->portDst, 2);
    for(int i = 0; i < MAX_ENTRIES; i++) {
        if(registryPort[i] == port) {
            (*(registryCallback[i]))(frame, flen);
            break;
        }
    }
}

int UDP_register(uint16_t port, CallbackUDP callback) {
    for(int i = 0; i < MAX_ENTRIES; i++) {
        if(registryPort[i] == port)
            return (registryCallback[i] == callback) ? 0 : -1;
    }
    for(int i = 0; i < MAX_ENTRIES; i++) {
        if(registryPort[i] == 0) {
            registryPort[i] = (port >> 8) | ((port & 0xFF) << 8);
            registryCallback[i] = callback;
            return 0;
        }
    }
    return -2;
}

int UDP_deregister(uint16_t port) {
    for(int i = 0; i < MAX_ENTRIES; i++) {
        if(registryPort[i] == port) {
            registryPort[i] = 0;
            registryCallback[i] = 0;
            return 0;
        }
    }
    return -1;
}
