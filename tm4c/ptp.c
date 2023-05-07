//
// Created by robert on 3/31/23.
//

#include <memory.h>
#include <math.h>
#include "hw/timer.h"
#include "lib/clk/mono.h"
#include "lib/clk/tai.h"
#include "lib/clk/util.h"
#include "lib/led.h"
#include "lib/net.h"
#include "lib/net/eth.h"
#include "lib/net/ip.h"
#include "lib/net/udp.h"
#include "lib/net/util.h"
#include "lib/ntp/ntp.h"
#include "lib/ntp/pll.h"
#include "ptp.h"


#define PTP2_ANNC_LOG_INTV (2) // 4 s
#define PTP2_SYNC_LOG_INTV (-2) // 4 Hz

#define PTP2_PORT_EVENT (319)
#define PTP2_PORT_GENERAL (320)
#define PTP2_MIN_SIZE (UDP_DATA_OFFSET + 34)
#define PTP2_ANNC_INTV (CLK_FREQ * 4) // 4 s
#define PTP2_SYNC_INTV (CLK_FREQ / 4) // 4 Hz
#define PTP2_MULTICAST (0x810100E0) // 224.0.1.129
#define PTP2_MULTICAST_PEER (0x6B0000E0) // 224.0.0.107
#define PTP2_VERSION (2)
#define PTP2_DOMAIN (1)


// PTP message types
enum PTP2_MTYPE {
    PTP2_MT_SYNC = 0x0,
    PTP2_MT_DELAY_REQ,
    PTP2_MT_PDELAY_REQ,
    PTP2_MT_PDELAY_RESP,

    PTP2_MT_FOLLOW_UP = 0x8,
    PTP2_MT_DELAY_RESP,
    PTP2_MT_PDELAY_RESP_FOLLOW_UP,
    PTP2_MT_ANNOUNCE,
    PTP2_MT_SIGNALING,
    PTP2_MT_MANAGEMENT
};

enum PTP2_CLK_CLASS {
    PTP2_CLK_CLASS_PRIMARY = 6,
    PTP2_CLK_CLASS_PRI_HOLD = 7,
    PTP2_CLK_CLASS_PRI_FAIL = 52
};

// PTP time source
enum PTP2_TSRC {
    PTP2_TSRC_ATOMIC = 0x10,
    PTP2_TSRC_GPS = 0x20,
    PTP2_TSRC_RADIO = 0x30,
    PTP2_TSRC_PTP = 0x40,
    PTP2_TSRC_NTP = 0x50,
    PTP2_TSRC_MANUAL = 0x60,
    PTP2_TSRC_OTHER = 0x90,
    PTP2_TSRC_INTERNAL = 0xA0
};

struct PACKED PTP2_SRC_IDENT {
    uint8_t identity[8];
    uint16_t portNumber;
};
_Static_assert(sizeof(struct PTP2_SRC_IDENT) == 10, "PTP_SRC_PORT must be 10 bytes");

struct PACKED PTP2_TIMESTAMP {
    uint16_t secondsHi;
    uint32_t secondsLo;
    uint32_t nanoseconds;
};
_Static_assert(sizeof(struct PTP2_TIMESTAMP) == 10, "PTP2_TIMESTAMP must be 10 bytes");

struct PACKED PTP2_HEADER {
    uint16_t messageType: 4;        // lower nibble
    uint16_t transportSpecific: 4;  // upper nibble
    uint16_t versionPTP: 4;         // lower nibble
    uint16_t reserved0: 4;          // upper nibble
    uint16_t messageLength;
    uint8_t domainNumber;
    uint8_t reserved1;
    uint16_t flags;
    uint64_t correctionField;
    uint32_t reserved2;
    struct PTP2_SRC_IDENT sourceIdentity;
    uint16_t sequenceId;
    uint8_t controlField;
    uint8_t logMessageInterval;
};
_Static_assert(sizeof(struct PTP2_HEADER) == 34, "PTP2_HEADER must be 34 bytes");

struct PACKED PTP2_ANNOUNCE {
    struct PTP2_TIMESTAMP originTimestamp;
    uint16_t currentUtcOffset;
    uint8_t reserved;
    uint8_t grandMasterPriority;
    uint32_t grandMasterClockQuality;
    uint8_t grandMasterPriority2;
    uint8_t grandMasterIdentity[8];
    uint16_t stepsRemoved;
    uint8_t timeSource;
};
_Static_assert(sizeof(struct PTP2_ANNOUNCE) == 30, "PTP2_ANNOUNCE must be 34 bytes");

typedef struct PTP2_TIMESTAMP PTP2_SYNC;
typedef struct PTP2_TIMESTAMP PTP2_DELAY_REQ;
typedef struct PTP2_TIMESTAMP PTP2_FOLLOW_UP;

struct PACKED PTP2_DELAY_RESP {
    struct PTP2_TIMESTAMP receiveTimestamp;
    struct PTP2_SRC_IDENT requestingIdentity;
};
_Static_assert(sizeof(struct PTP2_DELAY_RESP) == 20, "PTP2_DELAY_RESP must be 34 bytes");

struct PACKED PTP2_PDELAY_REQ {
    struct PTP2_TIMESTAMP receiveTimestamp;
    uint8_t reserved[10];
};
_Static_assert(sizeof(struct PTP2_PDELAY_REQ) == 20, "PTP2_PDELAY_REQ must be 34 bytes");

struct PACKED PTP2_PDELAY_RESP {
    struct PTP2_TIMESTAMP receiveReceiptTimestamp;
    struct PTP2_SRC_IDENT requestingIdentity;
};
_Static_assert(sizeof(struct PTP2_PDELAY_RESP) == 20, "PTP2_PDELAY_RESP must be 34 bytes");

struct PACKED PTP2_PDELAY_FOLLOW_UP {
    struct PTP2_TIMESTAMP responseOriginTimestamp;
    struct PTP2_SRC_IDENT requestingIdentity;
};
_Static_assert(sizeof(struct PTP2_PDELAY_RESP) == 20, "PTP2_PDELAY_FOLLOW_UP must be 34 bytes");


static uint32_t updatedSync, updatedAnnounce;
static uint8_t clockId[8];
static volatile uint32_t seqId;

static float lutClkAccuracy[17] = {
        25e-9f, 100e-9f, 250e-9f, 1e-6f,
        2.5e-6f, 10e-6f, 25e-6f, 100e-6f,
        250e-6f, 1e-3f, 2.5e-3f, 10e-3f,
        25e-6f, 100e-3f, 250e-3f, 1.0f,
        10.0f
};

static void processMessage(uint8_t *frame, int flen);
static void processDelayRequest(uint8_t *frame, int flen);
static void processPDelayRequest(uint8_t *frame, int flen);

static void sendAnnounce();
static void sendSync();

static void toPtpTimestamp(uint64_t ts, struct PTP2_TIMESTAMP *tsPtp);
static uint32_t toPtpClkAccuracy(float rmsError);

void PTP_init() {
    // set clock ID to MAC address
    getMAC(clockId + 2);
    // register UDP listening ports
    UDP_register(PTP2_PORT_EVENT, processMessage);
    UDP_register(PTP2_PORT_GENERAL, processMessage);
    // set next update event
    uint64_t now = CLK_MONO();
    updatedAnnounce = now;
    updatedSync = now;
}

void PTP_run() {
    // check for announce event
    if((GPTM0.TAV.raw - updatedAnnounce) >= PTP2_ANNC_INTV) {
        updatedAnnounce += PTP2_ANNC_INTV;
        sendAnnounce();
    }

    // check for sync event
    if((GPTM0.TAV.raw - updatedSync) >= PTP2_SYNC_INTV) {
        updatedSync += PTP2_SYNC_INTV;
        sendSync();
    }
}

void processMessage(uint8_t *frame, int flen) {
    // discard malformed packets
    if(flen < PTP2_MIN_SIZE) return;
    // map headers
    struct FRAME_ETH *headerEth = (struct FRAME_ETH *) frame;
    struct HEADER_IPv4 *headerIPv4 = (struct HEADER_IPv4 *) (headerEth + 1);
    struct HEADER_UDP *headerUDP = (struct HEADER_UDP *) (headerIPv4 + 1);
    struct PTP2_HEADER *headerPTP = (struct PTP2_HEADER *) (headerUDP + 1);

    // verify destination
    if(headerIPv4->dst != ipAddress) return;
    // ignore unsupported versions
    if(headerPTP->versionPTP != 2) return;

    // indicate time-server activity
    LED_act0();

    if(headerPTP->messageType == PTP2_MT_DELAY_REQ)
        return processDelayRequest(frame, flen);
    if(headerPTP->messageType == PTP2_MT_PDELAY_REQ)
        return processPDelayRequest(frame, flen);
}

static void processDelayRequest(uint8_t *frame, int flen) {

}

static void processPDelayRequest(uint8_t *frame, int flen) {

}

static void sendAnnounce() {
    int txDesc = NET_getTxDesc();
    if(txDesc < 0) return;
    // allocate and clear frame buffer
    const int flen = PTP2_MIN_SIZE + sizeof(struct PTP2_ANNOUNCE);
    uint8_t *frame = NET_getTxBuff(txDesc);
    memset(frame, 0, flen);

    // map headers
    struct FRAME_ETH *headerEth = (struct FRAME_ETH *) frame;
    struct HEADER_IPv4 *headerIPv4 = (struct HEADER_IPv4 *) (headerEth + 1);
    struct HEADER_UDP *headerUDP = (struct HEADER_UDP *) (headerIPv4 + 1);
    struct PTP2_HEADER *headerPTP = (struct PTP2_HEADER *) (headerUDP + 1);
    struct PTP2_ANNOUNCE *announce = (struct PTP2_ANNOUNCE *) (headerPTP + 1);

    // EtherType = IPv4
    headerEth->ethType = ETHTYPE_IPv4;

    // IPv4 Header
    IPv4_init(frame);
    IPv4_setMulticast(frame, PTP2_MULTICAST);
    headerIPv4->src = ipAddress;
    headerIPv4->proto = IP_PROTO_UDP;

    // UDP Header
    headerUDP->portSrc = __builtin_bswap16(PTP2_PORT_EVENT);
    headerUDP->portDst = __builtin_bswap16(PTP2_PORT_EVENT);

    // PTP header
    headerPTP->versionPTP = PTP2_VERSION;
    headerPTP->messageType = __builtin_bswap16(PTP2_MT_SYNC);
    headerPTP->messageLength = __builtin_bswap16(sizeof(struct PTP2_HEADER) + sizeof(struct PTP2_ANNOUNCE));
    memcpy(headerPTP->sourceIdentity.identity, clockId, sizeof(clockId));
    headerPTP->sourceIdentity.portNumber = __builtin_bswap16(PTP2_PORT_EVENT);
    headerPTP->domainNumber = PTP2_DOMAIN;
    headerPTP->logMessageInterval = PTP2_ANNC_LOG_INTV;
    headerPTP->sequenceId = __builtin_bswap16(seqId++);

    // PTP Announce
    announce->timeSource = (NTP_refId() == NTP_REF_GPS) ? PTP2_TSRC_GPS : PTP2_TSRC_NTP;
    announce->currentUtcOffset = (clkTaiUtcOffset >> 32);
    memcpy(announce->grandMasterIdentity, clockId, sizeof(clockId));
    announce->grandMasterPriority = 0;
    announce->grandMasterPriority2 = 0;
    announce->stepsRemoved = 0;
    announce->grandMasterClockQuality = toPtpClkAccuracy(PLL_offsetRms());
    toPtpTimestamp(CLK_TAI(), &(announce->originTimestamp));

    // transmit request
    UDP_finalize(frame, flen);
    IPv4_finalize(frame, flen);
    NET_transmit(txDesc, flen);
}

static void syncFollowup(void *ref, uint8_t *txFrame, int flen) {
    int txDesc = NET_getTxDesc();
    if(txDesc < 0) return;
    uint8_t *followup = NET_getTxBuff(txDesc);
    memcpy(followup, txFrame, flen);

    // get precise TX time
    uint64_t stamps[3];
    NET_getTxTime(txFrame, stamps);

    // map headers
    struct FRAME_ETH *headerEth = (struct FRAME_ETH *) followup;
    struct HEADER_IPv4 *headerIPv4 = (struct HEADER_IPv4 *) (headerEth + 1);
    struct HEADER_UDP *headerUDP = (struct HEADER_UDP *) (headerIPv4 + 1);
    struct PTP2_HEADER *headerPTP = (struct PTP2_HEADER *) (headerUDP + 1);
    PTP2_SYNC *sync = (PTP2_SYNC *) (headerPTP + 1);

    // set followup fields
    headerPTP->messageType = PTP2_MT_FOLLOW_UP;
    toPtpTimestamp(stamps[2], sync);

    // transmit request
    UDP_finalize(followup, flen);
    IPv4_finalize(followup, flen);
    NET_transmit(txDesc, flen);
}

static void sendSync() {
    int txDesc = NET_getTxDesc();
    if(txDesc < 0) return;
    // allocate and clear frame buffer
    const int flen = PTP2_MIN_SIZE + sizeof(PTP2_SYNC);
    uint8_t *frame = NET_getTxBuff(txDesc);
    memset(frame, 0, flen);

    // map headers
    struct FRAME_ETH *headerEth = (struct FRAME_ETH *) frame;
    struct HEADER_IPv4 *headerIPv4 = (struct HEADER_IPv4 *) (headerEth + 1);
    struct HEADER_UDP *headerUDP = (struct HEADER_UDP *) (headerIPv4 + 1);
    struct PTP2_HEADER *headerPTP = (struct PTP2_HEADER *) (headerUDP + 1);
    PTP2_SYNC *sync = (PTP2_SYNC *) (headerPTP + 1);

    // EtherType = IPv4
    headerEth->ethType = ETHTYPE_IPv4;

    // IPv4 Header
    IPv4_init(frame);
    IPv4_setMulticast(frame, PTP2_MULTICAST);
    headerIPv4->src = ipAddress;
    headerIPv4->proto = IP_PROTO_UDP;

    // UDP Header
    headerUDP->portSrc = __builtin_bswap16(PTP2_PORT_EVENT);
    headerUDP->portDst = __builtin_bswap16(PTP2_PORT_EVENT);

    headerPTP->versionPTP = PTP2_VERSION;
    headerPTP->messageType = __builtin_bswap16(PTP2_MT_SYNC);
    headerPTP->messageLength = __builtin_bswap16(sizeof(struct PTP2_HEADER) + sizeof(PTP2_SYNC));
    memcpy(headerPTP->sourceIdentity.identity, clockId, sizeof(clockId));
    headerPTP->sourceIdentity.portNumber = __builtin_bswap16(PTP2_PORT_EVENT);
    headerPTP->domainNumber = PTP2_DOMAIN;
    headerPTP->logMessageInterval = PTP2_SYNC_LOG_INTV;
    headerPTP->sequenceId = __builtin_bswap16(seqId++);

    // set preliminary timestamp
    toPtpTimestamp(CLK_TAI(), sync);

    // transmit request
    UDP_finalize(frame, flen);
    IPv4_finalize(frame, flen);
    NET_setTxCallback(txDesc, syncFollowup, NULL);
    NET_transmit(txDesc, flen);
}

static void toPtpTimestamp(uint64_t ts, struct PTP2_TIMESTAMP *tsPtp) {
    union fixed_32_32 scratch;
    scratch.full = ts;
    // set seconds
    tsPtp->secondsHi = 0;
    tsPtp->secondsLo = __builtin_bswap32(scratch.ipart);
    // convert fraction to nanoseconds
    scratch.ipart = 0;
    scratch.full *= 1000000000u;
    tsPtp->nanoseconds = __builtin_bswap32(scratch.ipart);
}

static uint32_t toPtpClkAccuracy(float rmsError) {
    // check accuracy thresholds
    for(int i = 0; i < 17; i++) {
        if(rmsError <= lutClkAccuracy[i])
            return 0x20 + i;
    }
    // greater than 10s
    return 0x31;
}
