//
// Created by robert on 4/26/22.
//

#ifndef GPSDO_NET_H
#define GPSDO_NET_H

#define PHY_STATUS_DUPLEX (4)
#define PHY_STATUS_100M (2)
#define PHY_STATUS_LINK (1)

typedef void (*CallbackNetTX)(uint8_t *frame, int flen, uint32_t tsSec, uint32_t tsNano);

void NET_init();
void NET_getLinkStatus(char *strStatus);
void NET_getMacAddress(char *strAddr);
void NET_getIpAddress(char *strAddr);
void NET_getIpSubnet(char *strAddr);
void NET_getIpGateway(char *strAddr);
void NET_getIpDNS(char *strAddr);
int NET_getPhyStatus();
void NET_run();

int NET_getTxDesc();
uint8_t * NET_getTxBuff(int desc);
void NET_setTxCallback(int desc, CallbackNetTX callback);
void NET_transmit(int desc, int len);

uint64_t NET_getRxTime(const uint8_t *rxFrame);
void NET_getRxTimeRaw(const uint8_t *rxFrame, uint32_t *rxTime);

#endif //GPSDO_NET_H
