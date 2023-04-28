//
// Created by robert on 4/27/23.
//

#ifndef GPSDO_PEER_H
#define GPSDO_PEER_H

#include "src.h"

#define PEER_BURST_SIZE (4)
#define PEER_BURST_RETRIES (2)
#define PEER_RESPONSE_TIMEOUT (0x40000000) // 256 ms
#define PEER_MIN_POLL (4)
#define PEER_MAX_POLL (16)

struct NtpPeer {
    struct NtpSource source;

    struct NtpPollSample burstSamples[PEER_BURST_SIZE];

    // packet timestamps
    uint64_t local_tx_hw;
    uint64_t local_tx;
    uint64_t remote_rx;
    uint64_t remote_tx;
    uint64_t local_rx;

    // state machine
    uint64_t lastPoll;
    int burstCounter;
    uint32_t lastArp;

    // remote mac address
    uint8_t macAddr[6];
};

void NtpPeer_init(void *pObj);

#endif //GPSDO_PEER_H
