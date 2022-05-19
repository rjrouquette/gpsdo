//
// Created by robert on 5/16/22.
//

#include "arp.h"
#include "eth.h"
#include "ip.h"
#include "../format.h"

volatile uint32_t ipAddress = 0;
volatile uint32_t ipSubnet = -1;
volatile uint32_t ipGateway = 0;
volatile uint32_t ipDNS = 0;


void NET_getIpAddress(char *strAddr) {
    strAddr += toDec((ipAddress >> 0u) & 0xFFu, 0, '0', strAddr);
    *(strAddr++) = '.';
    strAddr += toDec((ipAddress >> 8u) & 0xFFu, 0, '0', strAddr);
    *(strAddr++) = '.';
    strAddr += toDec((ipAddress >> 16u) & 0xFFu, 0, '0', strAddr);
    *(strAddr++) = '.';
    strAddr += toDec((ipAddress >> 24u) & 0xFFu, 0, '0', strAddr);
    *strAddr = 0;
}
