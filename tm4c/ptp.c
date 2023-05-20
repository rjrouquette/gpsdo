//
// Created by robert on 3/31/23.
//

#include <memory.h>
#include <math.h>
#include "hw/emac.h"
#include "lib/clk/tai.h"
#include "lib/clk/util.h"
#include "lib/led.h"
#include "lib/net.h"
#include "lib/net/eth.h"
#include "lib/net/util.h"
#include "lib/ntp/ntp.h"
#include "lib/ntp/pll.h"
#include "lib/run.h"
#include "ptp.h"


#define PTP2_ANNC_LOG_INTV (2) // 4 s
#define PTP2_SYNC_LOG_INTV (-2) // 4 Hz

#define PTP2_MIN_SIZE (sizeof(HEADER_ETH) + sizeof(HEADER_PTP))
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
    PTP2_MT_PDELAY_FOLLOW_UP,
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

typedef struct PACKED PTP2_SRC_IDENT {
    uint8_t identity[8];
    uint16_t portNumber;
} PTP2_SRC_IDENT;
_Static_assert(sizeof(struct PTP2_SRC_IDENT) == 10, "PTP2_SRC_IDENT must be 10 bytes");

typedef struct PACKED PTP2_TIMESTAMP {
    uint16_t secondsHi;
    uint32_t secondsLo;
    uint32_t nanoseconds;
} PTP2_TIMESTAMP;
_Static_assert(sizeof(struct PTP2_TIMESTAMP) == 10, "PTP2_TIMESTAMP must be 10 bytes");

typedef struct PACKED HEADER_PTP {
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
    PTP2_SRC_IDENT sourceIdentity;
    uint16_t sequenceId;
    uint8_t controlField;
    uint8_t logMessageInterval;
} HEADER_PTP;
_Static_assert(sizeof(struct HEADER_PTP) == 34, "HEADER_PTP must be 34 bytes");

typedef struct PACKED PTP2_ANNOUNCE {
    PTP2_TIMESTAMP originTimestamp;
    uint16_t currentUtcOffset;
    uint8_t reserved;
    uint8_t grandMasterPriority;
    uint32_t grandMasterClockQuality;
    uint8_t grandMasterPriority2;
    uint8_t grandMasterIdentity[8];
    uint16_t stepsRemoved;
    uint8_t timeSource;
} PTP2_ANNOUNCE;
_Static_assert(sizeof(struct PTP2_ANNOUNCE) == 30, "PTP2_ANNOUNCE must be 34 bytes");

typedef struct PACKED PTP2_DELAY_RESP {
    PTP2_TIMESTAMP receiveTimestamp;
    PTP2_SRC_IDENT requestingIdentity;
} PTP2_DELAY_RESP;
_Static_assert(sizeof(struct PTP2_DELAY_RESP) == 20, "PTP2_DELAY_RESP must be 34 bytes");

typedef struct PACKED PTP2_PDELAY_REQ {
    PTP2_TIMESTAMP receiveTimestamp;
    uint8_t reserved[10];
} PTP2_PDELAY_REQ;
_Static_assert(sizeof(struct PTP2_PDELAY_REQ) == 20, "PTP2_PDELAY_REQ must be 34 bytes");

typedef struct PACKED PTP2_PDELAY_RESP {
    PTP2_TIMESTAMP receiveTimestamp;
    PTP2_SRC_IDENT requestingIdentity;
} PTP2_PDELAY_RESP;
_Static_assert(sizeof(struct PTP2_PDELAY_RESP) == 20, "PTP2_PDELAY_RESP must be 34 bytes");

typedef struct PACKED PTP2_PDELAY_FOLLOW_UP {
    PTP2_TIMESTAMP responseTimestamp;
    PTP2_SRC_IDENT requestingIdentity;
} PTP2_PDELAY_FOLLOW_UP;
_Static_assert(sizeof(struct PTP2_PDELAY_RESP) == 20, "PTP2_PDELAY_FOLLOW_UP must be 34 bytes");


// IEEE 802.1AS broadcast MAC address (01:80:C2:00:00:0E)
static const uint8_t gPtpMac[6] = {0x01, 0x80, 0xC2, 0x00, 0x00, 0x0E };
static uint8_t clockId[8];
static volatile uint32_t seqId;

static float lutClkAccuracy[17] = {
        25e-9f, 100e-9f, 250e-9f, 1e-6f,
        2.5e-6f, 10e-6f, 25e-6f, 100e-6f,
        250e-6f, 1e-3f, 2.5e-3f, 10e-3f,
        25e-6f, 100e-3f, 250e-3f, 1.0f,
        10.0f
};

static void processDelayRequest(uint8_t *frame, int flen);
static void processPDelayRequest(uint8_t *frame, int flen);

static void sendAnnounce(void *ref);
static void sendSync(void *ref);

static void toPtpTimestamp(uint64_t ts, PTP2_TIMESTAMP *tsPtp);
static uint32_t toPtpClkAccuracy(float rmsError);

void PTP_init() {
    // set clock ID to MAC address
    getMAC(clockId + 2);

    // schedule periodic message transmission
    runSleep(1ull << (32 + PTP2_ANNC_LOG_INTV), sendAnnounce, NULL);
    runSleep(1ull << (32 + PTP2_SYNC_LOG_INTV), sendSync, NULL);
}

/**
 * Handles raw ethernet PTP frames
 * @param frame
 * @param flen
 */
void PTP_process(uint8_t *frame, int flen) {
    HEADER_ETH *headerEth = (HEADER_ETH *) frame;
    HEADER_PTP *headerPTP = (HEADER_PTP *) (headerEth + 1);

    // ignore anything we sent ourselves
    if(isMyMAC(headerEth->macSrc) == 0)
        return;
    // ignore unsupported versions
    if(headerPTP->versionPTP != 2) return;
    // flip mac address for reply
    copyMAC(headerEth->macDst, headerEth->macSrc);

    // indicate time-server activity
    LED_act0();

    if(headerPTP->messageType == PTP2_MT_DELAY_REQ)
        return processDelayRequest(frame, flen);
    if(headerPTP->messageType == PTP2_MT_PDELAY_REQ)
        return processPDelayRequest(frame, flen);
}

static void processDelayRequest(uint8_t *frame, int flen) {
    // verify request length
    if(flen < (PTP2_MIN_SIZE + sizeof(PTP2_TIMESTAMP)))
        return;

    int txDesc = NET_getTxDesc();
    if(txDesc < 0) return;
    // allocate and copy frame buffer
    uint8_t *txFrame = NET_getTxBuff(txDesc);
    memcpy(txFrame, frame, flen);
    // resize frame
    flen = PTP2_MIN_SIZE + sizeof(PTP2_DELAY_RESP);

    // map headers
    HEADER_ETH *headerEth = (HEADER_ETH *) txFrame;
    HEADER_PTP *headerPTP = (HEADER_PTP *) (headerEth + 1);
    PTP2_DELAY_RESP *resp = (PTP2_DELAY_RESP *) (headerPTP + 1);

    // copy source identity
    resp->requestingIdentity = headerPTP->sourceIdentity;
    // set RX time
    uint64_t stamps[3];
    NET_getRxTime(frame, stamps);
    toPtpTimestamp(stamps[2], &(resp->receiveTimestamp));

    // PTP header
    headerPTP->versionPTP = PTP2_VERSION;
    headerPTP->messageType = PTP2_MT_DELAY_RESP;
    headerPTP->messageLength = __builtin_bswap16(sizeof(HEADER_PTP) + sizeof(PTP2_DELAY_RESP));
    memcpy(headerPTP->sourceIdentity.identity, clockId, sizeof(clockId));
    headerPTP->sourceIdentity.portNumber = 0;
    headerPTP->domainNumber = PTP2_DOMAIN;
    headerPTP->logMessageInterval = 0;

    // transmit response
    NET_transmit(txDesc, flen);
}

static void peerDelayRespFollowup(void *ref, uint8_t *txFrame, int flen) {
    int txDesc = NET_getTxDesc();
    if(txDesc < 0) return;
    uint8_t *followup = NET_getTxBuff(txDesc);
    memcpy(followup, txFrame, flen);
    flen = PTP2_MIN_SIZE + sizeof(PTP2_PDELAY_FOLLOW_UP);

    // get precise TX time
    uint64_t stamps[3];
    NET_getTxTime(txFrame, stamps);

    // map headers
    HEADER_ETH *headerEth = (HEADER_ETH *) followup;
    HEADER_PTP *headerPTP = (HEADER_PTP *) (headerEth + 1);
    PTP2_PDELAY_FOLLOW_UP *followUp = (PTP2_PDELAY_FOLLOW_UP *) (headerPTP + 1);

    // set followup fields
    headerPTP->messageType = PTP2_MT_PDELAY_FOLLOW_UP;
    headerPTP->messageLength = __builtin_bswap16(sizeof(HEADER_PTP) + sizeof(PTP2_PDELAY_FOLLOW_UP));
    toPtpTimestamp(stamps[2], &(followUp->responseTimestamp));

    // transmit request
    NET_transmit(txDesc, flen);
}

static void processPDelayRequest(uint8_t *frame, int flen) {
    // verify request length
    if(flen < (PTP2_MIN_SIZE + sizeof(PTP2_PDELAY_REQ)))
        return;

    int txDesc = NET_getTxDesc();
    if(txDesc < 0) return;
    // allocate and copy frame buffer
    uint8_t *txFrame = NET_getTxBuff(txDesc);
    memcpy(txFrame, frame, flen);
    // resize frame
    flen = PTP2_MIN_SIZE + sizeof(PTP2_PDELAY_RESP);

    // map headers
    HEADER_ETH *headerEth = (HEADER_ETH *) txFrame;
    HEADER_PTP *headerPTP = (HEADER_PTP *) (headerEth + 1);
    PTP2_PDELAY_RESP *resp = (PTP2_PDELAY_RESP *) (headerPTP + 1);

    // copy source identity
    resp->requestingIdentity = headerPTP->sourceIdentity;
    // set RX time
    uint64_t stamps[3];
    NET_getRxTime(frame, stamps);
    toPtpTimestamp(stamps[2], &(resp->receiveTimestamp));

    // PTP header
    headerPTP->versionPTP = PTP2_VERSION;
    headerPTP->messageType = PTP2_MT_PDELAY_RESP;
    headerPTP->messageLength = __builtin_bswap16(sizeof(HEADER_PTP) + sizeof(PTP2_PDELAY_RESP));
    memcpy(headerPTP->sourceIdentity.identity, clockId, sizeof(clockId));
    headerPTP->sourceIdentity.portNumber = 0;
    headerPTP->domainNumber = PTP2_DOMAIN;
    headerPTP->logMessageInterval = 0;

    // transmit response
    NET_setTxCallback(txDesc, peerDelayRespFollowup, NULL);
    NET_transmit(txDesc, flen);
}

static void sendAnnounce(void *ref) {
    int txDesc = NET_getTxDesc();
    if(txDesc < 0) return;
    // allocate and clear frame buffer
    const int flen = PTP2_MIN_SIZE + sizeof(PTP2_ANNOUNCE);
    uint8_t *frame = NET_getTxBuff(txDesc);
    memset(frame, 0, flen);

    // map headers
    HEADER_ETH *headerEth = (HEADER_ETH *) frame;
    HEADER_PTP *headerPTP = (HEADER_PTP *) (headerEth + 1);
    PTP2_ANNOUNCE *announce = (PTP2_ANNOUNCE *) (headerPTP + 1);

    // IEEE 802.1AS
    headerEth->ethType = ETHTYPE_PTP;
    copyMAC(headerEth->macDst, gPtpMac);

    // PTP header
    headerPTP->versionPTP = PTP2_VERSION;
    headerPTP->messageType = PTP2_MT_ANNOUNCE;
    headerPTP->messageLength = __builtin_bswap16(sizeof(HEADER_PTP) + sizeof(PTP2_ANNOUNCE));
    memcpy(headerPTP->sourceIdentity.identity, clockId, sizeof(clockId));
    headerPTP->sourceIdentity.portNumber = 0;
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
    HEADER_ETH *headerEth = (HEADER_ETH *) followup;
    HEADER_PTP *headerPTP = (HEADER_PTP *) (headerEth + 1);
    PTP2_TIMESTAMP *sync = (PTP2_TIMESTAMP *) (headerPTP + 1);

    // set followup fields
    headerPTP->messageType = PTP2_MT_FOLLOW_UP;
    toPtpTimestamp(stamps[2], sync);

    // transmit request
    NET_transmit(txDesc, flen);
}

static void sendSync(void *ref) {
    int txDesc = NET_getTxDesc();
    if(txDesc < 0) return;
    // allocate and clear frame buffer
    const int flen = PTP2_MIN_SIZE + sizeof(PTP2_TIMESTAMP);
    uint8_t *frame = NET_getTxBuff(txDesc);
    memset(frame, 0, flen);

    // map headers
    HEADER_ETH *headerEth = (HEADER_ETH *) frame;
    HEADER_PTP *headerPTP = (HEADER_PTP *) (headerEth + 1);
    PTP2_TIMESTAMP *sync = (PTP2_TIMESTAMP *) (headerPTP + 1);

    // IEEE 802.1AS
    headerEth->ethType = ETHTYPE_PTP;
    copyMAC(headerEth->macDst, gPtpMac);

    headerPTP->versionPTP = PTP2_VERSION;
    headerPTP->messageType = PTP2_MT_SYNC;
    headerPTP->messageLength = __builtin_bswap16(sizeof(HEADER_PTP) + sizeof(PTP2_TIMESTAMP));
    memcpy(headerPTP->sourceIdentity.identity, clockId, sizeof(clockId));
    headerPTP->sourceIdentity.portNumber = 0;
    headerPTP->domainNumber = PTP2_DOMAIN;
    headerPTP->logMessageInterval = PTP2_SYNC_LOG_INTV;
    headerPTP->sequenceId = __builtin_bswap16(seqId++);

    // set preliminary timestamp
    toPtpTimestamp(CLK_TAI(), sync);

    // transmit request
    NET_setTxCallback(txDesc, syncFollowup, NULL);
    NET_transmit(txDesc, flen);
}

static void toPtpTimestamp(uint64_t ts, PTP2_TIMESTAMP *tsPtp) {
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
