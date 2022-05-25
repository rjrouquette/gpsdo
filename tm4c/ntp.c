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
#include "lib/led.h"
#include "gpsdo.h"
#include "hw/emac.h"

#define NTP_PORT (123)

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

static volatile uint32_t ntpEra = 0;
static volatile uint64_t ntpTimeOffset = 0xe6338cbb00000000;

void NTP_process(uint8_t *frame, int flen);

void NTP_process0(const uint8_t *frame, struct FRAME_NTPv3 *frameNTP);
void NTP_followup0(uint8_t *frame, int flen, uint32_t txSec, uint32_t txNano);

void NTP_process3(const uint8_t *frame, struct FRAME_NTPv3 *frameNTP);

void NTP_init() {
    UDP_register(NTP_PORT, NTP_process);
}

uint64_t NTP_offset() {
    return ntpTimeOffset;
}

void NTP_date(uint64_t clkMono, uint32_t *ntpDate) {
    uint64_t rollover = ((uint32_t *)&clkMono)[1];
    rollover += ((uint32_t *)&ntpTimeOffset)[1];
    clkMono += ntpTimeOffset;
    ntpDate[0] = ntpEra + ((uint32_t *)&rollover)[1];
    ntpDate[1] = __builtin_bswap32(((uint32_t *)&clkMono)[1]);
    ntpDate[2] = __builtin_bswap32(((uint32_t *)&clkMono)[0]);
    ntpDate[3] = 0;
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
    // ignore non-client frames
    if(frameNTP->flags.mode != 0x03) return;
    // time-server activity
    LED_act0();

    // modify ethernet frame header
    copyMAC(headerEth->macDst, headerEth->macSrc);
    // modify IP header
    headerIPv4->dst = headerIPv4->src;
    headerIPv4->src = ipAddress;
    // modify UDP header
    headerUDP->portDst = headerUDP->portSrc;
    headerUDP->portSrc = __builtin_bswap16(NTP_PORT);

    // get TX descriptor
    int txDesc = NET_getTxDesc();
    if(txDesc < 0) return;

    // process message
    if(frameNTP->flags.version == 0) {
        NTP_process0(frame, frameNTP);
        NET_setTxCallback(txDesc, NTP_followup0);
    }
    else {
        NTP_process3(frame, frameNTP);
    }

    // finalize packet
    UDP_finalize(frame, flen);
    IPv4_finalize(frame, flen);
    // transmit packet
    memcpy(NET_getTxBuff(txDesc), frame, flen);
    NET_transmit(txDesc, flen);
}

void NTP_process0(const uint8_t *frame, struct FRAME_NTPv3 *frameNTP) {
    // retrieve rx time
    uint32_t rxTime[2];
    NET_getRxTimeRaw(frame, rxTime);

    // set type to server response
    frameNTP->flags.mode = 4;
    // indicate that the time is not currently set
    frameNTP->flags.status = GPSDO_isLocked() ? 0 : 3;
    // set other header fields
    frameNTP->stratum = GPSDO_isLocked() ? 1 : 16;
    frameNTP->precision = -24;
    // set reference ID
    memcpy(frameNTP->refID, "GPS", 4);
    // set root delay
    frameNTP->rootDelay = 0;
    // set root dispersion
    frameNTP->rootDispersion = 0;
    // set origin timestamp
    frameNTP->origTime[0] = frameNTP->txTime[0];
    frameNTP->origTime[1] = frameNTP->txTime[1];
    // set RX time
    frameNTP->rxTime[0] = ((uint32_t *) &rxTime)[0];
    frameNTP->rxTime[1] = ((uint32_t *) &rxTime)[1];
    // set reference time
    uint32_t refTime[2] = {EMAC0.TIMSEC, EMAC0.TIMNANO };
    if((int32_t)(refTime[1] - rxTime[1]) < 0) {
        if(refTime[0] == rxTime[0]) ++refTime[0];
    }
    frameNTP->refTime[0] = refTime[0];
    frameNTP->refTime[1] = refTime[1];
    // no TX time
    frameNTP->txTime[0] = 0;
    frameNTP->txTime[1] = 0;
}

void NTP_followup0(uint8_t *frame, int flen, uint32_t txSec, uint32_t txNano) {
    // discard malformed packets
    if(flen < 90) return;

    // map headers
    struct FRAME_ETH *headerEth = (struct FRAME_ETH *) frame;
    struct HEADER_IPv4 *headerIPv4 = (struct HEADER_IPv4 *) (headerEth + 1);
    struct HEADER_UDP *headerUDP = (struct HEADER_UDP *) (headerIPv4 + 1);
    struct FRAME_NTPv3 *frameNTP = (struct FRAME_NTPv3 *) (headerUDP + 1);

    // guard against buffer overruns
    if(headerEth->ethType != ETHTYPE_IPv4) return;
    if(headerIPv4->proto != IP_PROTO_UDP) return;
    if(headerUDP->portSrc != __builtin_bswap16(NTP_PORT)) return;
    if(frameNTP->flags.version != 0) return;

    // append original TX time and resend
    frameNTP->txTime[0] = txSec;
    frameNTP->txTime[1] = txNano;

    // send packet
    UDP_finalize(frame, flen);
    IPv4_finalize(frame, flen);
    // get TX descriptor
    int txDesc = NET_getTxDesc();
    if(txDesc < 0) return;
    // get new TX buffer
    uint8_t *newFrame = NET_getTxBuff(txDesc);
    // copy frame to new TX buffer (but only if the new buffer has a different address)
    if(newFrame != frame)
        memcpy(newFrame, frame, flen);
    // transmit response
    NET_transmit(txDesc, flen);
}

void NTP_process3(const uint8_t *frame, struct FRAME_NTPv3 *frameNTP) {
    // retrieve rx time
    uint64_t rxTime = NET_getRxTime(frame) + ntpTimeOffset;

    // set type to server response
    frameNTP->flags.mode = 4;
    // indicate that the time is not currently set
    frameNTP->flags.status = GPSDO_isLocked() ? 0 : 3;
    // set other header fields
    frameNTP->stratum = GPSDO_isLocked() ? 1 : 16;
    frameNTP->precision = -24;
    // set reference ID
    memcpy(frameNTP->refID, "GPS", 4);
    // set root delay
    frameNTP->rootDelay = 0;
    // set root dispersion
    frameNTP->rootDispersion = 0;
    // set origin timestamp
    frameNTP->origTime[0] = frameNTP->txTime[0];
    frameNTP->origTime[1] = frameNTP->txTime[1];
    // set reference timestamp
    uint64_t refTime = CLK_TAI() + ntpTimeOffset;
    frameNTP->refTime[0] = __builtin_bswap32(((uint32_t *) &refTime)[1]);
    frameNTP->refTime[1] = __builtin_bswap32(((uint32_t *) &refTime)[0]);
    // set RX time
    frameNTP->rxTime[0] = __builtin_bswap32(((uint32_t *) &rxTime)[1]);
    frameNTP->rxTime[1] = __builtin_bswap32(((uint32_t *) &rxTime)[0]);
    // set TX time
    uint64_t txTime = CLK_TAI() + ntpTimeOffset;
    frameNTP->txTime[0] = __builtin_bswap32(((uint32_t *) &txTime)[1]);
    frameNTP->txTime[1] = __builtin_bswap32(((uint32_t *) &txTime)[0]);
}
