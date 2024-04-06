//
// Created by robert on 4/26/22.
//

#ifndef GPSDO_NET_H
#define GPSDO_NET_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define PHY_STATUS_DUPLEX (4)
#define PHY_STATUS_100M (2)
#define PHY_STATUS_LINK (1)

typedef void (*CallbackNetTX)(void *ref, uint8_t *frame, int flen);

void NET_init();
void NET_getMacAddress(char *strAddr);
int NET_getPhyStatus();

int NET_getTxDesc();
uint8_t * NET_getTxBuff(int desc);
void NET_setTxCallback(int desc, CallbackNetTX callback, volatile void *ref);
void NET_transmit(int desc, int len);

void NET_getRxTime(volatile uint64_t *stamps);

void NET_getTxTime(const uint8_t *txFrame, volatile uint64_t *stamps);

#ifdef __cplusplus
}
#endif

#endif //GPSDO_NET_H
