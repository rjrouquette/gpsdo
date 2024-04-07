//
// Created by robert on 3/26/24.
//

#pragma once

#include "source.hpp"

namespace ntp {
    class GPS final : public Source {
        uint64_t lastPoll;
        uint64_t lastPps;

        static void run(void *ref);

        void run();

        void updateStatus();

    public:
        GPS();

        ~GPS();
    };
}
