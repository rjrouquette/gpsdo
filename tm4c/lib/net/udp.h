//
// Created by robert on 5/20/22.
//

#ifndef GPSDO_UDP_H
#define GPSDO_UDP_H

#ifdef __cplusplus
extern "C" {
#else
#define static_assert _Static_assert
#endif

#include <stdint.h>

#ifndef PACKED
#define PACKED __attribute__((packed))
#endif

typedef struct PACKED HEADER_UDP {
    uint16_t portSrc;
    uint16_t portDst;
    uint16_t length;
    uint16_t chksum;
} HEADER_UDP;
static_assert(sizeof(struct HEADER_UDP) == 8, "HEADER_UDP must be 8 bytes");

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
 * Modify headers from a received frame to create a direct reply to the sender
 * @param frame raw frame buffer
 * @param ipAddr new source address
 * @param port new source port
 */
void UDP_returnToSender(uint8_t *frame, uint32_t ipAddr, uint16_t port);

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

#ifdef __cplusplus
}
#endif

#endif //GPSDO_UDP_H
