//
// Created by robert on 5/20/22.
//

#pragma once

#include "ip.hpp"


struct [[gnu::packed]] HeaderUdp4 {
    uint16_t portSrc;
    uint16_t portDst;
    uint16_t length;
    uint16_t chksum;
};

static_assert(sizeof(HeaderUdp4) == 8, "HeaderUdp must be 8 bytes");

struct [[gnu::packed]] FrameUdp4 : FrameIp4 {
    static constexpr int DATA_OFFSET = FrameIp4::DATA_OFFSET + sizeof(HeaderUdp4);

    HeaderUdp4 udp;

    static auto& from(void *frame) {
        return *static_cast<FrameUdp4*>(frame);
    }

    static auto& from(const void *frame) {
        return *static_cast<const FrameUdp4*>(frame);
    }

    void returnToSender();

    void returnToSender(uint32_t ipAddr, uint16_t port);
};

static_assert(sizeof(FrameUdp4) == 42, "FrameUdp4 must be 42 bytes");

#define UDP_DATA_OFFSET (14+20+8)

typedef void (*CallbackUDP)(uint8_t *frame, int flen);

/**
 * Process received UDP frame
 * @param frame raw frame buffer
 * @param flen raw frame length
 */
void UDP_process(uint8_t *frame, int flen);

/**
 * Finalize raw UDP frame for transmission
 * @param frame raw frame buffer
 * @param flen raw frame buffer length
 */
void UDP_finalize(uint8_t *frame, int flen);

/**
 * Register callback to receive inbound UDP port traffic
 * @param port port number to register
 * @param callback function to receive data
 * @return 0 on success, -1 on conflict, -2 if port table is full
 */
int UDP_register(uint16_t port, CallbackUDP callback);

/**
 * Delete UDP port registration
 * @param port port number to deregister
 * @return 0 on success, -1 if port was not registered
 */
int UDP_deregister(uint16_t port);
