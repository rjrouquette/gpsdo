//
// Created by robert on 3/31/23.
//

#include <memory.h>
#include <math.h>

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
#define PTP2_ANNOUNCE_INTERVAL (1ull << (32 + 4))
#define PTP2_SYNC_INTERVAL (1ull << (32 - 4))
#define PTP2_MULTICAST (0x810100E0) // 224.0.1.129
#define PTP2_MULTICAST_PEER (0x6B0000E0) // 224.0.0.107


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


static uint64_t nextSync, nextAnnounce;
static uint8_t clockId[8];

static void processMessage(uint8_t *frame, int flen);
static void processDelayRequest(uint8_t *frame, int flen);
static void processPDelayRequest(uint8_t *frame, int flen);

static void sendAnnounce();
static void sendSync();

void PTP_init() {
    // set clock ID to MAC address
    getMAC(clockId + 2);
    // register UDP listening ports
    UDP_register(PTP2_PORT_EVENT, processMessage);
    UDP_register(PTP2_PORT_GENERAL, processMessage);
    // set next update event
    uint64_t now = CLK_MONOTONIC();
    nextAnnounce = now + PTP2_ANNOUNCE_INTERVAL;
    nextSync = now + PTP2_SYNC_INTERVAL;
}

void PTP_run() {
    const uint64_t now = CLK_MONOTONIC();

    // check for sync event
    if(((int64_t)(now - nextSync)) >= 0) {
        nextSync += PTP2_SYNC_INTERVAL;
        sendSync();
    }

    // check for announce event
    if(((int64_t)(now - nextAnnounce)) >= 0) {
        nextAnnounce += PTP2_ANNOUNCE_INTERVAL;
        sendAnnounce();
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

    // TODO set payload

    // transmit request
    UDP_finalize(frame, flen);
    IPv4_finalize(frame, flen);
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

    // TODO set payload

    // transmit request
    UDP_finalize(frame, flen);
    IPv4_finalize(frame, flen);
    NET_transmit(txDesc, flen);
}
