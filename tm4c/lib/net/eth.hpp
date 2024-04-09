//
// Created by robert on 5/16/22.
//

#pragma once

#include <cstddef>
#include <cstdint>

constexpr uint16_t ETHTYPE_ARP = 0x0608;
constexpr uint16_t ETHTYPE_IP4 = 0x0008;
constexpr uint16_t ETHTYPE_IP6 = 0xDD86;
constexpr uint16_t ETHTYPE_PTP = 0xF788;

struct [[gnu::packed]] HeaderEthernet {
    uint8_t macDst[6];
    uint8_t macSrc[6];
    uint16_t ethType;
};

static_assert(sizeof(HeaderEthernet) == 14, "HeaderEthernet must be 14 bytes");

struct [[gnu::packed]] FrameEthernet {
    static constexpr int DATA_OFFSET = sizeof(HeaderEthernet);

    HeaderEthernet eth;

    static auto& from(void *frame) {
        return *static_cast<FrameEthernet*>(frame);
    }

    static auto& from(const void *frame) {
        return *static_cast<const FrameEthernet*>(frame);
    }
};

static_assert(sizeof(FrameEthernet) == sizeof(HeaderEthernet), "FrameEthernet must be 14 bytes");
