//
// Created by robert on 4/26/22.
//

#ifndef GPSDO_NET_H
#define GPSDO_NET_H

void NET_init();
void NET_getMacAddress(char *strAddr);
void NET_getLinkStatus(char *strStatus);
int NET_readyPacket();

#endif //GPSDO_NET_H
