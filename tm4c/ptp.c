//
// Created by robert on 3/31/23.
//

#include "ptp.h"
#include "lib/clk.h"
#include "lib/net.h"
#include "lib/net/eth.h"
#include "lib/net/ip.h"
#include "lib/net/udp.h"
#include "lib/net/util.h"
#include "lib/led.h"


#define PTP2_PORT_EVENT (319)
#define PTP2_PORT_GENERAL (320)
#define PTP2_MIN_SIZE (UDP_DATA_OFFSET + 34)


// PTP message yypes
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



static void processMessage(uint8_t *frame, int flen);
static void processDelayRequest(uint8_t *frame, int flen);
static void processPDelayRequest(uint8_t *frame, int flen);

void PTP_init() {
    UDP_register(PTP2_PORT_EVENT, processMessage);
    UDP_register(PTP2_PORT_GENERAL, processMessage);
}

void PTP_run() {

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
