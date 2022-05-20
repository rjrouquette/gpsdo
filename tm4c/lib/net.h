//
// Created by robert on 4/26/22.
//

#ifndef GPSDO_NET_H
#define GPSDO_NET_H

void NET_init();
void NET_getLinkStatus(char *strStatus);
void NET_getMacAddress(char *strAddr);
void NET_getIpAddress(char *strAddr);
void NET_poll();

int NET_getTxDesc();
uint8_t * NET_getTxBuff(int desc);
void NET_transmit(int desc, int len);

#endif //GPSDO_NET_H
