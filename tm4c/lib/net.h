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

uint32_t NET_packetsRX();
uint32_t NET_packetsTX();

#endif //GPSDO_NET_H
