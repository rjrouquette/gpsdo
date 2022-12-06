//
// Created by robert on 4/26/22.
//

#ifndef GPSDO_NET_H
#define GPSDO_NET_H

#define PHY_STATUS_DUPLEX (4)
#define PHY_STATUS_100M (2)
#define PHY_STATUS_LINK (1)

typedef void (*CallbackNetTX)(uint8_t *frame, int flen);

void NET_init();
void NET_getMacAddress(char *strAddr);
int NET_getPhyStatus();
void NET_run();

int NET_getTxDesc();
uint8_t * NET_getTxBuff(int desc);
void NET_setTxCallback(int desc, CallbackNetTX callback);
void NET_transmit(int desc, int len);

uint64_t NET_getRxTime(const uint8_t *rxFrame);
void NET_getRxTimeRaw(const uint8_t *rxFrame, uint32_t *rxTime);

uint64_t NET_getTxTime(const uint8_t *txFrame);
void NET_getTxTimeRaw(const uint8_t *txFrame, uint32_t *txTime);

#endif //GPSDO_NET_H
