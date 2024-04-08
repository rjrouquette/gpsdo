//
// Created by robert on 3/26/24.
//

#include "peer.hpp"

#include "common.hpp"
#include "../net.h"
#include "../rand.h"
#include "../run.h"
#include "../clk/mono.h"
#include "../clk/tai.h"
#include "../clk/util.h"
#include "../net/arp.h"
#include "../net/eth.h"
#include "../net/ip.h"
#include "../net/util.h"

#include <memory.h>

#define IDLE_INTV (RUN_SEC >> 1)
#define ACTV_INTV (RUN_SEC >> 4)

inline uint64_t avg64(const uint64_t a, const uint64_t b) {
    return a + (static_cast<int64_t>(b - a) >> 1);
}

ntp::Peer::Peer(const uint32_t ipAddr) :
    Source(ipAddr, RPY_SD_MD_CLIENT),
    local_tx_hw{}, local_rx_hw{}, macAddr{} {
    // clear state variables
    filterRx = 0;
    filterTx = 0;
    remote_rx = 0;
    remote_tx = 0;
    pollActive = false;
    pollStart = 0;
    pollTrim = 0;
    pollWait = 0;
    pollXleave = false;
    lastArp = 0;
    pktRecv = false;
    pktSent = false;

    // initialize polling
    if (!IPv4_testSubnet(ipSubnet, ipAddress, id)) {
        // faster initial burst for local timeservers
        maxPoll = 4;
        minPoll = 2;
        poll = 1;
    }
    else {
        maxPoll = PEER_MAX_POLL;
        minPoll = PEER_MIN_POLL;
        poll = PEER_MIN_POLL;
    }
    // start source updates
    this->taskHandle = runSleep(IDLE_INTV, &run, this);
}

ntp::Peer::~Peer() {
    Source::~Source();
}

void ntp::Peer::run(void *ref) {
    static_cast<Peer*>(ref)->run();
}

void ntp::Peer::run() {
    // check if a poll is currently active
    if (pollActive) {
        if (pollRun())
            return;

        // add random fuzz to polling interval to temporally disperse poll requests
        // (maximum of 1/16 of polling interval)
        fixed_32_32 scratch = {};
        scratch.fpart = 0;
        scratch.ipart = RAND_next();
        scratch.full >>= 36 - poll;
        scratch.full |= 1ull << (32 + poll);
        // schedule next update
        pollWait = scratch.ipart;
        pollTrim = scratch.fpart >> 8;
        runAdjust(taskHandle, scratch.ipart ? RUN_SEC : (scratch.fpart >> 8));
        return;
    }

    // requires hardware time synchronization
    // requires valid MAC address
    if (clkMonoEth == 0 || isMacInvalid()) {
        runAdjust(taskHandle, IDLE_INTV);
        return;
    }

    if (pollWait) {
        if (--pollWait == 0)
            runAdjust(taskHandle, pollTrim);
        return;
    }

    // start poll
    pollStart = CLK_MONO();
    pollActive = true;
    pollXleave = false;
    filterTx = 0;
    filterRx = 0;
    reach <<= 1;
    // send poll packet
    pollSend();
    pktSent = true;
    runAdjust(taskHandle, ACTV_INTV);
}

void ntp::Peer::updateStatus() {
    // clear lost flag if peer was reached
    if (reach & 0xF)
        lost = false;

    // increment poll counter
    if (++pollCounter == 0)
        pollCounter = -1;

    // adjust poll interval
    if (
        sampleCount == MAX_HISTORY &&
        (reach & 0xFF) == 0xFF &&
        pollCounter >= 8 &&
        poll < maxPoll
    ) {
        // reset the counter
        pollCounter = 0;
        // increase poll interval (increase update rate)
        ++poll;
        // sanity check
        if (poll < minPoll)
            poll = minPoll;
    }
    else if (
        (reach & 0xF) == 0 &&
        pollCounter >= 4
    ) {
        // mark association as lost
        lost = true;
        // reset the counter
        pollCounter = 0;
        // decrease poll interval (increase update rate)
        if (poll > minPoll)
            --poll;
    }

    // mark source for pruning if it is unreachable
    prune = (reach == 0) && (pollCounter >= 16);
    // mark source for pruning if its stratum is too high
    prune |= stratum > MAX_STRATUM;
    // mark source for pruning if its delay is too high
    prune |= (usedOffset > 7) && (delayMean > MAX_DELAY);
}

bool ntp::Peer::pollRun() {
    // send packet
    if (!pktSent) {
        pktSent = true;
        pktRecv = false;
        pollSend();
        return true;
    }

    // receive packet
    if (pktRecv) {
        pktRecv = false;
        // send followup interleaved request for local timeservers
        if (!pollXleave && !IPv4_testSubnet(ipSubnet, ipAddress, id)) {
            pollXleave = true;
            pollSend();
            return true;
        }
        // complete poll
        pollEnd();
    }
    else {
        uint64_t elapsed = CLK_MONO() - pollStart;
        if (elapsed > PEER_RESPONSE_TIMEOUT) {
            // fallback to original response if we were waiting for an interleaved followup
            if (pollXleave) {
                xleave = false;
                // complete poll
                pollEnd();
            }
            // response timeout
            pollActive = false;
        }
    }

    // end of burst
    if (!pollActive) {
        // update status
        updateStatus();
        return false;
    }
    return true;
}

void ntp::Peer::pollEnd() {
    // drop sample if source is not synchronized
    if (stratum > 15)
        return;
    // set reach bit
    reach |= 1;

    // translate remote timestamps
    const uint64_t remote_rx = htonll(this->remote_rx) + clkTaiUtcOffset - NTP_UTC_OFFSET;
    const uint64_t remote_tx = htonll(this->remote_tx) + clkTaiUtcOffset - NTP_UTC_OFFSET;

    // advance sample buffer
    auto &sample = advanceFilter();
    // set compensated reference time
    const uint64_t comp = avg64(local_tx_hw[1], local_rx_hw[1]);
    sample.comp = comp;
    // set TAI reference time
    const uint64_t tai = avg64(local_tx_hw[2], local_rx_hw[2]);
    sample.taiSkew = tai - comp;
    // compute TAI offset
    const uint64_t offset = avg64(remote_rx, remote_tx) - tai;
    sample.offset = static_cast<int64_t>(offset);
    // compute delay (use compensated clock for best accuracy)
    const uint64_t responseTime = remote_tx - remote_rx;
    const auto delay = static_cast<int64_t>((local_rx_hw[1] - local_tx_hw[1]) - responseTime);
    sample.delay = 0x1p-31f * static_cast<float>(static_cast<int32_t>(delay >> 2));
    // set response time
    this->responseTime = toFloat(static_cast<int64_t>(responseTime));

    // update filter
    updateFilter();
    // poll complete
    pollActive = false;
}

void ntp::Peer::pollSend() {
    const int txDesc = NET_getTxDesc();
    // allocate and clear frame buffer
    uint8_t *frame = NET_getTxBuff(txDesc);
    memset(frame, 0, NTP4_SIZE);

    // map headers
    const auto headerEth = reinterpret_cast<HEADER_ETH*>(frame);
    const auto headerIP4 = reinterpret_cast<HEADER_IP4*>(headerEth + 1);
    const auto headerUDP = reinterpret_cast<HEADER_UDP*>(headerIP4 + 1);
    const auto headerNTP = reinterpret_cast<HEADER_NTP*>(headerUDP + 1);

    // MAC address
    copyMAC(headerEth->macDst, macAddr);

    // IPv4 Header
    IPv4_init(frame);
    headerIP4->dst = id;
    headerIP4->src = ipAddress;
    headerIP4->proto = IP_PROTO_UDP;

    // UDP Header
    headerUDP->portSrc = htons(NTP_PORT_CLI);
    headerUDP->portDst = htons(NTP_PORT_SRV);

    // set type to client request
    headerNTP->version = 4;
    headerNTP->mode = 3;
    headerNTP->poll = poll;
    // set stratum and precision
    headerNTP->stratum = 16;
    headerNTP->precision = NTP_CLK_PREC;
    // set reference ID
    headerNTP->refID = 0;
    // set reference timestamp
    headerNTP->refTime = 0;
    if (pollXleave) {
        // set filter timestamps
        filterRx = htonll(local_rx_hw[0]);
        filterTx = htonll(local_tx_hw[0]);
        // interleaved query
        headerNTP->origTime = remote_rx;
        headerNTP->rxTime = filterRx;
        headerNTP->txTime = filterTx;
    }
    else {
        // set filter timestamps
        filterRx = 0;
        filterTx = htonll(CLK_MONO());
        // standard query
        headerNTP->origTime = 0;
        headerNTP->rxTime = filterRx;
        headerNTP->txTime = filterTx;
        // set callback for tx timestamp
        NET_setTxCallback(txDesc, txCallback, this);
    }

    // transmit request
    ++txCount;
    UDP_finalize(frame, NTP4_SIZE);
    IPv4_finalize(frame, NTP4_SIZE);
    NET_transmit(txDesc, NTP4_SIZE);
}

bool ntp::Peer::isMacInvalid() {
    // refresh stale MAC addresses
    if (!lastArp || (CLK_MONO_INT() - lastArp) > ARP_MAX_AGE)
        updateMac();
    // report if MAC address is invalid
    return isNullMAC(macAddr);
}

void ntp::Peer::updateMac() {
    lastArp = CLK_MONO_INT();
    if (IPv4_testSubnet(ipSubnet, ipAddress, id))
        copyMAC(const_cast<uint8_t*>(macAddr), macRouter);
    else
        ARP_request(id, arpCallback, this);
}

// ARP response callback
void ntp::Peer::arpCallback(void *ref, uint32_t remoteAddr, const uint8_t *macAddr) {
    // typecast "this" pointer
    auto *instance = static_cast<ntp::Peer*>(ref);
    if (isNullMAC(macAddr)) {
        // retry if request timed-out
        instance->updateMac();
    }
    else {
        // update MAC address
        copyMAC(const_cast<uint8_t*>(instance->macAddr), macAddr);
    }
}

void ntp::Peer::txCallback(void *ref, uint8_t *frame, int flen) {
    // set hardware timestamp
    NET_getTxTime(frame, static_cast<Peer*>(ref)->local_tx_hw);
}

void ntp::Peer::receive(uint8_t *frame, int flen) {
    ++rxCount;

    // map headers
    const auto headerEth = reinterpret_cast<HEADER_ETH*>(frame);
    const auto headerIP4 = reinterpret_cast<HEADER_IP4*>(headerEth + 1);
    const auto headerUDP = reinterpret_cast<HEADER_UDP*>(headerIP4 + 1);
    const auto headerNTP = reinterpret_cast<HEADER_NTP*>(headerUDP + 1);

    // check for interleaved response
    if (pollXleave) {
        // discard non-interleaved packets
        if (headerNTP->origTime == filterTx) {
            // packet received, but it was not interleaved
            pktRecv = true;
            xleave = false;
            return;
        }
        // discard unsolicited packets
        if (headerNTP->origTime != filterRx)
            return;
        // replace remote TX timestamp with updated value
        remote_tx = headerNTP->txTime;
        // packet received, and it was interleaved
        pktRecv = true;
        xleave = true;
        // increment count of valid packets
        ++rxValid;
        return;
    }

    // drop unsolicited packets
    if (headerNTP->origTime != filterTx)
        return;
    // prevent reception of duplicate packets
    filterTx = 0;
    // packet received
    pktRecv = true;
    // set hardware timestamp
    NET_getRxTime(local_rx_hw);

    // set stratum
    stratum = headerNTP->stratum;
    leap = headerNTP->status;
    precision = headerNTP->precision;
    version = headerNTP->version;
    ntpMode = headerNTP->mode;
    refId = headerNTP->refID;
    refTime = headerNTP->refTime;
    // set remote timestamps
    remote_rx = headerNTP->rxTime;
    remote_tx = headerNTP->txTime;
    // set root metrics
    rootDelay = htonl(headerNTP->rootDelay);
    rootDispersion = htonl(headerNTP->rootDispersion);
    // increment count of valid packets
    ++rxValid;
}
