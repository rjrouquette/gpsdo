//
// Created by robert on 4/27/23.
//

#ifndef GPSDO_PEER_H
#define GPSDO_PEER_H

#include <stdbool.h>
#include "src.h"

#define PEER_RESPONSE_TIMEOUT (0x80000000ull) // 500 ms
#define PEER_MIN_POLL (4)
#define PEER_MAX_POLL (6)

struct NtpPeer {
    struct NtpSource source;

    // packet timestamps
    uint64_t local_tx_hw[3];
    uint64_t local_tx;
    uint64_t remote_rx;
    uint64_t remote_tx;
    uint64_t local_rx;
    uint64_t local_rx_hw[3];

    // state machine
    uint64_t nextPoll;
    uint64_t pollStart;
    uint32_t lastArp;

    uint8_t pollBurst;
    uint8_t pollRetry;
    bool pollActive;
    bool pollXleave;
    bool pktSent;
    bool pktRecv;

    // remote mac address
    const uint8_t macAddr[6];
};
typedef volatile struct NtpPeer NtpPeer;

/**
 * Initialize peer structure
 * @param pObj pointer to peer structure
 */
void NtpPeer_init(volatile void *pObj);

/**
 * Process received frame for peer
 * @param pObj pointer to peer structure
 * @param frame pointer to received frame
 * @param flen length of received frame
 */
void NtpPeer_recv(volatile void *pObj, uint8_t *frame, int flen);

#endif //GPSDO_PEER_H
