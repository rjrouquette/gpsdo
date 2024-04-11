//
// Created by robert on 3/26/24.
//

#pragma once

#include "source.hpp"

namespace ntp {
    class Peer final : public Source {
        // 500 ms (32.32 fixed point)
        static constexpr uint64_t PEER_RESPONSE_TIMEOUT = 1ull << 31;
        static constexpr int PEER_MIN_POLL = 4;
        static constexpr int PEER_MAX_POLL = 6;

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

        static void run(void *ref);

        void run();

        void updateStatus();

        bool pollRun();

        void pollEnd();

        void pollSend();

        bool isMacInvalid();

        void updateMac();

        static void arpCallback(void *ref, uint32_t remoteAddr, const uint8_t *macAddr);

        static void txCallback(void *ref, uint8_t *frame, int flen);

    public:
        explicit Peer(uint32_t ipAddr);

        ~Peer();

        void receive(uint8_t *frame, int flen);
    };
}
