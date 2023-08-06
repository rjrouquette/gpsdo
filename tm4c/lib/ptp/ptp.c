//
// Created by robert on 3/31/23.
//

#include <memory.h>
#include <math.h>
#include "../clk/tai.h"
#include "../led.h"
#include "../net.h"
#include "../net/eth.h"
#include "../net/util.h"
#include "../ntp/ntp.h"
#include "../ntp/pll.h"
#include "../run.h"
#include "common.h"
#include "ptp.h"


#define PTP2_ANNC_LOG_INTV (0) // 1 s
#define PTP2_SYNC_LOG_INTV (-4) // 16 Hz

uint8_t ptpClockId[8];
static volatile uint32_t seqId;

static void processDelayRequest(uint8_t *frame, int flen);
static void processPDelayRequest(uint8_t *frame, int flen);

static void sendAnnounce(void *ref);
static void sendSync(void *ref);

void PTP_init() {
    // set clock ID to MAC address
    getMAC(ptpClockId + 2);

    // schedule periodic message transmission
    runSleep(RUN_SEC >> -PTP2_ANNC_LOG_INTV, sendAnnounce, NULL);
    runSleep(RUN_SEC >> -PTP2_SYNC_LOG_INTV, sendSync, NULL);
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
    memcpy(headerPTP->sourceIdentity.identity, ptpClockId, sizeof(ptpClockId));
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
    memcpy(headerPTP->sourceIdentity.identity, ptpClockId, sizeof(ptpClockId));
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
    memcpy(headerPTP->sourceIdentity.identity, ptpClockId, sizeof(ptpClockId));
    headerPTP->sourceIdentity.portNumber = 0;
    headerPTP->domainNumber = PTP2_DOMAIN;
    headerPTP->logMessageInterval = PTP2_ANNC_LOG_INTV;
    headerPTP->sequenceId = __builtin_bswap16(seqId++);

    // PTP Announce
    announce->timeSource = (NTP_refId() == NTP_REF_GPS) ? PTP2_TSRC_GPS : PTP2_TSRC_NTP;
    announce->currentUtcOffset = (clkTaiUtcOffset >> 32);
    memcpy(announce->grandMasterIdentity, ptpClockId, sizeof(ptpClockId));
    announce->grandMasterPriority = 0;
    announce->grandMasterPriority2 = 0;
    announce->stepsRemoved = 0;
    if(NTP_refId())
        announce->grandMasterClockQuality = toPtpClkAccuracy(PLL_offsetRms());
    else
        announce->grandMasterClockQuality = 0x31;
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
    memcpy(headerPTP->sourceIdentity.identity, ptpClockId, sizeof(ptpClockId));
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
