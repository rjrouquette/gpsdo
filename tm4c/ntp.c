//
// Created by robert on 5/21/22.
//

#include <memory.h>

#include "ntp.h"
#include "lib/clk.h"
#include "lib/net.h"
#include "lib/net/eth.h"
#include "lib/net/ip.h"
#include "lib/net/udp.h"
#include "lib/net/util.h"

struct PACKED FRAME_NTPv3 {
    union PACKED {
        struct PACKED {
            unsigned mode: 3;
            unsigned version: 3;
            unsigned status: 2;
        };
        uint8_t bits;
    } flags;
    uint8_t stratum;
    uint8_t poll;
    int8_t precision;
    uint32_t rootDelay;
    uint32_t rootDispersion;
    uint8_t refID[4];
    uint32_t refTime[2];
    uint32_t origTime[2];
    uint32_t rxTime[2];
    uint32_t txTime[2];
};
_Static_assert(sizeof(struct FRAME_NTPv3) == 48, "FRAME_NTPv3 must be 48 bytes");

static volatile uint64_t ntpTimeOffset = 0xe6338cbb00000000;

void NTP_process(uint8_t *frame, int flen);

void NTP_init() {
    UDP_register(123, NTP_process);
}

void NTP_process(uint8_t *frame, int flen) {
    // discard malformed packets
    if(flen < 90) return;
    // map headers
    struct FRAME_ETH *headerEth = (struct FRAME_ETH *) frame;
    struct HEADER_IPv4 *headerIPv4 = (struct HEADER_IPv4 *) (headerEth + 1);
    struct HEADER_UDP *headerUDP = (struct HEADER_UDP *) (headerIPv4 + 1);
    struct FRAME_NTPv3 *frameNTP = (struct FRAME_NTPv3 *) (headerUDP + 1);

    // verify destination
    if(headerIPv4->dst != ipAddress) return;

    uint64_t rxTime = CLK_MONOTONIC() + ntpTimeOffset;

    // ignore non-client frames
    if(frameNTP->flags.mode != 0x03) return;
    // modify ethernet frame header
    copyMAC(headerEth->macDst, headerEth->macSrc);
    // modify IP header
    headerIPv4->dst = headerIPv4->src;
    headerIPv4->src = ipAddress;
    // modify UDP header
    headerUDP->portDst = headerUDP->portSrc;
    headerUDP->portSrc = __builtin_bswap16(123);
    // set type to server response
    frameNTP->flags.mode = 4;
    // indicate that the time is not currently set
    frameNTP->flags.status = 3;
    // set other header fields
    frameNTP->stratum = 1;
    frameNTP->precision = -24;
    // set reference ID
    memcpy(frameNTP->refID, "GPS", 4);
    // set root delay
    frameNTP->rootDelay = 0;
    // set root dispersion
    frameNTP->rootDispersion = __builtin_bswap32(1);
    // set origin timestamp
    frameNTP->origTime[0] = frameNTP->txTime[0];
    frameNTP->origTime[1] = frameNTP->txTime[1];
    // set reference timestamp
    uint64_t refTime = CLK_MONOTONIC() + ntpTimeOffset;
    frameNTP->refTime[0] = __builtin_bswap32(((uint32_t *) &refTime)[1]);
    frameNTP->refTime[1] = __builtin_bswap32(((uint32_t *) &refTime)[0]);
    // set RX time
    frameNTP->rxTime[0] = __builtin_bswap32(((uint32_t *) &rxTime)[1]);
    frameNTP->rxTime[1] = __builtin_bswap32(((uint32_t *) &rxTime)[0]);
    // set TX time
    uint64_t txTime = CLK_MONOTONIC() + ntpTimeOffset;
    frameNTP->txTime[0] = __builtin_bswap32(((uint32_t *) &txTime)[1]);
    frameNTP->txTime[1] = __builtin_bswap32(((uint32_t *) &txTime)[0]);

    // send packet
    UDP_finalize(frame, flen);
    IPv4_finalize(frame, flen);
    // get TX descriptor
    int txDesc = NET_getTxDesc();
    if(txDesc < 0) return;
    // transmit response
    memcpy(NET_getTxBuff(txDesc), frame, flen);
    NET_transmit(txDesc, flen);
}
