//
// Created by robert on 4/26/22.
//

#pragma once

#include <cstdint>

#define PHY_STATUS_DUPLEX (4)
#define PHY_STATUS_100M (2)
#define PHY_STATUS_LINK (1)

void NET_getMacAddress(char *strAddr);
int NET_getPhyStatus();

namespace network {
    using CallbackTx = void (*)(void *ref, const uint8_t *frame, int size);

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

    /**
     * Submit an ethernet frame for transmission.
     * @param frame the data to transmit
     * @param size the size of the frame to transmit
     * @param callback function to invoke when transmission is complete
     * @param ref pointer to reference data for callback
     * @return true if the frame was added to the transmit buffer, false if the frame would overrun the transmit buffer
     */
    bool transmit(const uint8_t *frame, int size, CallbackTx callback = nullptr, void *ref = nullptr);
}
