//
// Created by robert on 3/31/23.
//

#include "ptp.hpp"

#include "common.hpp"
#include "../led.hpp"
#include "../net.hpp"
#include "../run.hpp"
#include "../clock/tai.hpp"
#include "../net/util.hpp"
#include "../ntp/GPS.hpp"
#include "../ntp/ntp.hpp"
#include "../ntp/pll.hpp"

#include <algorithm>
#include <memory.h>


#define PTP2_ANNC_LOG_INTV (0) // 1 s
#define PTP2_SYNC_LOG_INTV (-4) // 16 Hz

uint8_t ptpClockId[8];
static volatile uint32_t seqId;

static void processDelayRequest(const uint8_t *frame, int size);
static void processPDelayRequest(const uint8_t *frame, int size);

static void sendAnnounce(void *ref);
static void sendSync(void *ref);

void ptp::init() {
    // set clock ID to MAC address
    getMAC(ptpClockId + 2);

    // schedule periodic message transmission
    runSleep(RUN_SEC >> -PTP2_ANNC_LOG_INTV, sendAnnounce, nullptr);
    runSleep(RUN_SEC >> -PTP2_SYNC_LOG_INTV, sendSync, nullptr);
}

/**
 * Handles raw ethernet PTP frames
 * @param frame
 * @param size
 */
void PTP_process(uint8_t *frame, const int size) {
    auto &packet = PacketPTP<char>::from(frame);

    // ignore anything we sent ourselves
    if (isMyMAC(packet.eth.macSrc) == 0)
        return;
    // ignore unsupported versions
    if (packet.ptp.versionPTP != 2)
        return;
    // flip mac address for reply
    copyMAC(packet.eth.macDst, packet.eth.macSrc);

    // indicate time-server activity
    LED_act0();

    if (packet.ptp.messageType == PTP2_MT_DELAY_REQ)
        return processDelayRequest(frame, size);
    if (packet.ptp.messageType == PTP2_MT_PDELAY_REQ)
        return processPDelayRequest(frame, size);
}

static void processDelayRequest(const uint8_t *frame, int size) {
    // verify request length
    if (size < static_cast<int>(PTP2_MIN_SIZE + sizeof(PTP2_TIMESTAMP)))
        return;

    // copy and resize frame
    constexpr int txSize = PTP2_MIN_SIZE + sizeof(PTP2_DELAY_RESP);
    uint8_t txFrame[txSize] = {};
    memcpy(txFrame, frame, std::min(size, txSize));

    // map headers
    auto &response = PacketPTP<PTP2_DELAY_RESP>::from(txFrame);

    // copy source identity
    response.data.requestingIdentity = response.ptp.sourceIdentity;
    // set RX time
    uint64_t stamps[3];
    network::getRxTime(stamps);
    toPtpTimestamp(stamps[2], &(response.data.receiveTimestamp));

    // PTP header
    response.ptp.versionPTP = PTP2_VERSION;
    response.ptp.messageType = PTP2_MT_DELAY_RESP;
    response.ptp.messageLength = htons(sizeof(HEADER_PTP) + sizeof(PTP2_DELAY_RESP));
    memcpy(response.ptp.sourceIdentity.identity, ptpClockId, sizeof(ptpClockId));
    response.ptp.sourceIdentity.portNumber = 0;
    response.ptp.domainNumber = PTP2_DOMAIN;
    response.ptp.logMessageInterval = 0;

    // transmit response
    network::transmit(txFrame, txSize);
}

static void peerDelayRespFollowup(void *ref, const uint8_t *frame, int size) {
    // get precise TX time
    uint64_t stamps[3];
    network::getTxTime(stamps);

    // copy and resize frame
    constexpr int txSize = PTP2_MIN_SIZE + sizeof(PTP2_PDELAY_FOLLOW_UP);
    uint8_t txFrame[txSize] = {};
    memcpy(txFrame, frame, std::min(size, txSize));

    // map headers
    auto &response = PacketPTP<PTP2_PDELAY_FOLLOW_UP>::from(txFrame);

    // set followup fields
    response.ptp.messageType = PTP2_MT_PDELAY_FOLLOW_UP;
    response.ptp.messageLength = htons(sizeof(HEADER_PTP) + sizeof(PTP2_PDELAY_FOLLOW_UP));
    toPtpTimestamp(stamps[2], &(response.data.responseTimestamp));

    // transmit request
    network::transmit(txFrame, txSize);
}

static void processPDelayRequest(const uint8_t *frame, int size) {
    // verify request length
    if (size < static_cast<int>(PTP2_MIN_SIZE + sizeof(PTP2_PDELAY_REQ)))
        return;

    // copy and resize frame
    constexpr int txSize = PTP2_MIN_SIZE + sizeof(PTP2_PDELAY_RESP);
    uint8_t txFrame[txSize] = {};
    memcpy(txFrame, frame, std::min(size, txSize));

    // map headers
    auto &response = PacketPTP<PTP2_PDELAY_RESP>::from(txFrame);

    // copy source identity
    response.data.requestingIdentity = response.ptp.sourceIdentity;
    // set RX time
    uint64_t stamps[3];
    network::getRxTime(stamps);
    toPtpTimestamp(stamps[2], &(response.data.receiveTimestamp));

    // PTP header
    response.ptp.versionPTP = PTP2_VERSION;
    response.ptp.messageType = PTP2_MT_PDELAY_RESP;
    response.ptp.messageLength = htons(sizeof(HEADER_PTP) + sizeof(PTP2_PDELAY_RESP));
    memcpy(response.ptp.sourceIdentity.identity, ptpClockId, sizeof(ptpClockId));
    response.ptp.sourceIdentity.portNumber = 0;
    response.ptp.domainNumber = PTP2_DOMAIN;
    response.ptp.logMessageInterval = 0;

    // transmit response
    network::transmit(txFrame, txSize, peerDelayRespFollowup, nullptr);
}

static void sendAnnounce(void *ref) {
    // allocate and clear frame buffer
    constexpr int size = PTP2_MIN_SIZE + sizeof(PTP2_ANNOUNCE);
    uint8_t frame[size] = {};

    // map headers
    auto &announce = PacketPTP<PTP2_ANNOUNCE>::from(frame);

    // IEEE 802.1AS
    announce.eth.ethType = ETHTYPE_PTP;
    copyMAC(announce.eth.macDst, gPtpMac);

    // PTP header
    announce.ptp.versionPTP = PTP2_VERSION;
    announce.ptp.messageType = PTP2_MT_ANNOUNCE;
    announce.ptp.messageLength = htons(sizeof(HEADER_PTP) + sizeof(PTP2_ANNOUNCE));
    memcpy(announce.ptp.sourceIdentity.identity, ptpClockId, sizeof(ptpClockId));
    announce.ptp.sourceIdentity.portNumber = 0;
    announce.ptp.domainNumber = PTP2_DOMAIN;
    announce.ptp.logMessageInterval = PTP2_ANNC_LOG_INTV;
    announce.ptp.sequenceId = htons(seqId++);

    // PTP Announce
    const auto refId = ntp::refId();
    announce.data.timeSource = (refId == ntp::GPS::REF_ID) ? PTP2_TSRC_GPS : PTP2_TSRC_NTP;
    announce.data.currentUtcOffset = (clkTaiUtcOffset >> 32);
    memcpy(announce.data.grandMasterIdentity, ptpClockId, sizeof(ptpClockId));
    announce.data.grandMasterPriority = 0;
    announce.data.grandMasterPriority2 = 0;
    announce.data.stepsRemoved = 0;
    if (refId != 0)
        announce.data.grandMasterClockQuality = toPtpClkAccuracy(PLL_offsetRms());
    else
        announce.data.grandMasterClockQuality = 0x31;
    toPtpTimestamp(clock::tai::now(), &announce.data.originTimestamp);

    // transmit announce frame
    network::transmit(frame, size);
}

static void syncFollowup(void *ref, const uint8_t *frame, const int size) {
    // get precise TX time
    uint64_t stamps[3];
    network::getTxTime(stamps);

    // copy frame
    uint8_t txFrame[size];
    memcpy(txFrame, frame, size);

    // map headers
    auto &followup = PacketPTP<PTP2_TIMESTAMP>::from(txFrame);

    // set followup fields
    followup.ptp.messageType = PTP2_MT_FOLLOW_UP;
    toPtpTimestamp(stamps[2], &followup.data);

    // transmit request
    network::transmit(txFrame, size);
}

static void sendSync(void *ref) {
    // allocate and clear frame buffer
    constexpr int size = PTP2_MIN_SIZE + sizeof(PTP2_TIMESTAMP);
    uint8_t frame[size] = {};

    // map headers
    auto &sync = PacketPTP<PTP2_TIMESTAMP>::from(frame);

    // IEEE 802.1AS
    sync.eth.ethType = ETHTYPE_PTP;
    copyMAC(sync.eth.macDst, gPtpMac);

    sync.ptp.versionPTP = PTP2_VERSION;
    sync.ptp.messageType = PTP2_MT_SYNC;
    sync.ptp.messageLength = htons(sizeof(HEADER_PTP) + sizeof(PTP2_TIMESTAMP));
    memcpy(sync.ptp.sourceIdentity.identity, ptpClockId, sizeof(ptpClockId));
    sync.ptp.sourceIdentity.portNumber = 0;
    sync.ptp.domainNumber = PTP2_DOMAIN;
    sync.ptp.logMessageInterval = PTP2_SYNC_LOG_INTV;
    sync.ptp.sequenceId = htons(seqId++);

    // set preliminary timestamp
    toPtpTimestamp(clock::tai::now(), &sync.data);

    // transmit sync frame
    network::transmit(frame, size, syncFollowup, nullptr);
}
