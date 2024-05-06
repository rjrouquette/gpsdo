//
// Created by robert on 4/26/22.
//

#pragma once

#include <cstdint>

#define PHY_STATUS_DUPLEX (4)
#define PHY_STATUS_100M (2)
#define PHY_STATUS_LINK (1)

typedef void (*CallbackNetTX)(void *ref, uint8_t *frame, int flen);

void NET_getMacAddress(char *strAddr);
int NET_getPhyStatus();

int NET_getTxDesc();
uint8_t* NET_getTxBuff(int desc);
void NET_setTxCallback(int desc, CallbackNetTX callback, volatile void *ref);
void NET_transmit(int desc, int len);

namespace network {
    /**
     * Initialize network stack.
     */
    void init();

    /**
     * Get the number of ethernet receiver DMA overflow events
     * @return the number ethernet receiver DMA overflow events
     */
    uint32_t getOverflowRx();

    /**
     * Get the link-level receive timestamp associated with the current callback context.
     * @param stamps array of length 3 that will be populated with
     *               64-bit fixed-point format (32.32) timestamps <br/>
     *               0 - monotonic clock <br/>
     *               1 - compensated clock <br/>
     *               2 - TAI clock
     */
    void getRxTime(uint64_t *stamps);

    /**
     * Get the link-level transmit timestamp associated with the current callback context.
     * @param stamps array of length 3 that will be populated with
     *               64-bit fixed-point format (32.32) timestamps <br/>
     *               0 - monotonic clock <br/>
     *               1 - compensated clock <br/>
     *               2 - TAI clock
     */
    void getTxTime(uint64_t *stamps);
}
