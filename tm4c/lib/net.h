//
// Created by robert on 4/26/22.
//

#ifndef GPSDO_NET_H
#define GPSDO_NET_H

volatile uint16_t phyStatus;

void NET_init();
void NET_getMacAddress(char *strAddr);
int NET_readyPacket();

#endif //GPSDO_NET_H
