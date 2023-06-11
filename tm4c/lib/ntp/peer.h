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
    NtpSource source;

    // packet timestamp filters
    uint64_t filterTx;
    uint64_t filterRx;
    // packet timestamps
    uint64_t local_tx_hw[3];
    uint64_t remote_rx;
    uint64_t remote_tx;
    uint64_t local_rx_hw[3];

    // state machine
    uint64_t pollStart;
    uint32_t pollWait;
    uint32_t pollTrim;
    uint32_t lastArp;

    // task handle
    void *taskHandle;

    bool pollActive;
    bool pollXleave;
    bool pktSent;
    bool pktRecv;

    // remote mac address
    const uint8_t macAddr[6];
};
typedef struct NtpPeer NtpPeer;

/**
 * Initialize peer structure
 * @param pObj pointer to peer structure
 */
void NtpPeer_init(void *pObj);

/**
 * Process received frame for peer
 * @param pObj pointer to peer structure
 * @param frame pointer to received frame
 * @param flen length of received frame
 */
void NtpPeer_recv(void *pObj, uint8_t *frame, int flen);

/**
 * Run peer tasks
 * @param pObj pointer to peer structure
 */
void NtpPeer_run(void *pObj);

#endif //GPSDO_PEER_H
