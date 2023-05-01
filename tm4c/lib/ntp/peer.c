//
// Created by robert on 4/27/23.
//

#include <memory.h>
#include <math.h>
#include "../chrony/candm.h"
#include "../clk/mono.h"
#include "../clk/tai.h"
#include "../clk/util.h"
#include "../rand.h"
#include "../net.h"
#include "../net/arp.h"
#include "../net/eth.h"
#include "../net/util.h"
#include "common.h"
#include "peer.h"


// average two 64-bit numbers
static uint64_t avg64(uint64_t a, uint64_t b);

// API callbacks
static void arpCallback(void *ref, uint32_t remoteAddr, uint8_t *macAddr);
static void txCallback(void *ref, uint8_t *frame, int flen);

// private member functions
static void updateMac(NtpPeer *this);
static int checkMac(NtpPeer *this);
static void sendPoll(NtpPeer *this);
static void startPoll(NtpPeer *this);
static void runPoll(NtpPeer *this);
static void finishPoll(NtpPeer *this);


static uint64_t avg64(uint64_t a, uint64_t b) {
    int64_t diff = (int64_t) (b - a);
    return a + (diff >> 1);
}

// ARP response callback
static void arpCallback(void *ref, uint32_t remoteAddr, uint8_t *macAddr) {
    // typecast "this" pointer
    NtpPeer *this = (struct NtpPeer *) ref;
    if(isNullMAC(macAddr)) {
        // retry if request timed-out
        updateMac(this);
    } else {
        // update MAC address
        copyMAC((uint8_t *) this->macAddr, macAddr);
    }
}

static void updateMac(NtpPeer *this) {
    this->lastArp = CLK_MONO_INT();
    if(IPv4_testSubnet(ipSubnet, ipAddress, this->source.id))
        copyMAC((uint8_t *) this->macAddr, macRouter);
    else
        ARP_request(this->source.id, arpCallback, this);
}

// Checks MAC address of peer and performs ARP request to retrieve it
static int checkMac(NtpPeer *this) {
    // refresh stale MAC addresses
    if(!this->lastArp || (CLK_MONO_INT() - this->lastArp) > ARP_MAX_AGE)
        updateMac(this);
    // report if MAC address is invalid
    return isNullMAC(this->macAddr) ? 1 : 0;
}

static void txCallback(void *ref, uint8_t *frame, int flen) {
    // typecast "this" pointer
    struct NtpPeer *this = (struct NtpPeer *) ref;
    // set hardware timestamp
    NET_getTxTime(frame, this->local_tx_hw);
}

static void sendPoll(NtpPeer *this) {
    int txDesc = NET_getTxDesc();
    if(txDesc < 0) return;
    // allocate and clear frame buffer
    uint8_t *frame = NET_getTxBuff(txDesc);
    memset(frame, 0, NTP4_SIZE);

    // map headers
    struct FRAME_ETH *headerEth = (struct FRAME_ETH *) frame;
    struct HEADER_IPv4 *headerIPv4 = (struct HEADER_IPv4 *) (headerEth + 1);
    struct HEADER_UDP *headerUDP = (struct HEADER_UDP *) (headerIPv4 + 1);
    struct HEADER_NTPv4 *headerNTP = (struct HEADER_NTPv4 *) (headerUDP + 1);

    // EtherType = IPv4
    headerEth->ethType = ETHTYPE_IPv4;
    // MAC address
    copyMAC(headerEth->macDst, this->macAddr);

    // IPv4 Header
    IPv4_init(frame);
    headerIPv4->dst = this->source.id;
    headerIPv4->src = ipAddress;
    headerIPv4->proto = IP_PROTO_UDP;

    // UDP Header
    headerUDP->portSrc = __builtin_bswap16(NTP_PORT_CLI);
    headerUDP->portDst = __builtin_bswap16(NTP_PORT_SRV);

    // set type to client request
    headerNTP->version = 4;
    headerNTP->mode = 3;
    headerNTP->poll = this->source.poll;
    // set stratum and precision
    headerNTP->stratum = 16;
    headerNTP->precision = NTP_CLK_PREC;
    // set reference ID
    headerNTP->refID = 0;
    // set reference timestamp
    headerNTP->refTime = 0;
    if(this->pollXleave) {
        // interleaved timestamp chaining
        headerNTP->origTime = this->remote_rx;
        headerNTP->rxTime = this->local_rx_hw[0];
        headerNTP->txTime = this->local_tx_hw[0];
    } else {
        // standard timestamp chaining
        headerNTP->origTime = this->remote_tx;
        headerNTP->rxTime = __builtin_bswap64((this->local_rx_hw[2] - clkTaiUtcOffset) + NTP_UTC_OFFSET);
        // set provisional TX time (actual value in unimportant, only need to be unique)
        uint64_t txTime = CLK_MONO();
        headerNTP->txTime = txTime;
        this->local_tx = txTime;
        // set callback for tx timestamp
        NET_setTxCallback(txDesc, txCallback, this);
    }

    // transmit request
    ++this->source.txCount;
    UDP_finalize(frame, NTP4_SIZE);
    IPv4_finalize(frame, NTP4_SIZE);
    NET_transmit(txDesc, NTP4_SIZE);
}

static void startPoll(NtpPeer *this) {
    if(this->pollActive) {
        // prior poll never completed
        NtpSource_updateStatus(&(this->source));
    }

    // add random fuzz to polling interval to temporally disperse poll requests
    // (maximum of 1/16 of polling interval)
    union fixed_32_32 scratch;
    scratch.fpart = 0;
    scratch.ipart = RAND_next();
    scratch.full >>= 36 - this->source.poll;
    scratch.full |= 1ull << (32 + this->source.poll);
    // set next poll
    this->pollNext += scratch.full;

    // start poll
    this->pollStart = CLK_MONO();
    this->pollActive = true;
    this->pollBurst = 0xFF;
    this->pollRetry = 0;
    this->pollXleave = false;
    this->local_tx = 0;
}

static void runPoll(NtpPeer *this) {
    // start of poll
    if(this->pollBurst == 0xFF) {
        this->source.reach <<= 1;
        this->pollBurst = 0;
        this->pktSent = false;
    }

    // send packet
    if(!this->pktSent) {
        this->pktSent = true;
        this->pktRecv = false;
        sendPoll(this);
        return;
    }

    // receive packet
    if(this->pktRecv) {
        this->pktRecv = false;
        // send followup interleaved request for local timeservers
        if(!this->pollXleave && !IPv4_testSubnet(ipSubnet, ipAddress, this->source.id)) {
            this->pollXleave = true;
            sendPoll(this);
            return;
        }
        // complete poll
        finishPoll(this);
    } else {
        uint64_t now = CLK_MONO();
        if((now - this->pollStart) > PEER_RESPONSE_TIMEOUT) {
            // fallback to original response if we were waiting for an interleaved followup
            if(this->pollXleave) {
                this->source.xleave = false;
                // complete poll
                finishPoll(this);
            }
            // response timeout
            this->pollActive = false;
        }
    }

    // end of burst
    if(!this->pollActive) {
        // update status
        NtpSource_updateStatus(&(this->source));
    }
}

static void finishPoll(NtpPeer *this) {
    // drop sample if source is not synchronized
    if(this->source.stratum > 15)
        return;
    // set reach bit
    this->source.reach |= 1;

    // translate remote timestamps
    uint64_t remote_rx = __builtin_bswap64(this->remote_rx) + clkTaiUtcOffset - NTP_UTC_OFFSET;
    uint64_t remote_tx = __builtin_bswap64(this->remote_tx) + clkTaiUtcOffset - NTP_UTC_OFFSET;

    // advance sample buffer
    NtpSource_incr(&this->source);
    // set current sample
    NtpPollSample *sample = this->source.pollSample + this->source.samplePtr;
    // set compensated reference time
    sample->comp = avg64(this->local_tx_hw[1], this->local_rx_hw[1]);
    // set TAI reference time
    sample->tai = avg64(this->local_tx_hw[2], this->local_rx_hw[2]);
    // compute TAI offset
    uint64_t offset = avg64(remote_rx, remote_tx) - sample->tai;
    sample->offset = (int64_t) offset;
    // compute delay (use compensated clock for best accuracy)
    uint64_t responseTime = remote_tx - remote_rx;
    int64_t delay = (int64_t) ((this->local_rx_hw[1] - this->local_tx_hw[1]) - responseTime);
    sample->delay = 0x1p-31f * (float) (int32_t) (delay >> 2);
    // set response time
    this->source.responseTime = toFloat((int64_t) responseTime);

    // update filter
    NtpSource_update(&this->source);
    // poll complete
    this->pollActive = false;
}


static void run(volatile void *pObj) {
    // typecast "this" pointer
    NtpPeer *this = (NtpPeer *) pObj;

    // check MAC address
    if(checkMac(this)) return;

    // check for start of poll
    const uint64_t now = CLK_MONO();
    if(((int64_t)(now - this->pollNext)) > 0)
        startPoll(this);

    // run current poll
    if(this->pollActive)
        runPoll(this);
}

void NtpPeer_init(volatile void *pObj) {
    // clear structure contents
    memset((void *) pObj, 0, sizeof(struct NtpPeer));

    // typecast "this" pointer
    NtpPeer *this = (NtpPeer *) pObj;
    // set virtual functions
    this->source.init = NtpPeer_init;
    this->source.run = run;
    // set mode and state
    this->source.mode = RPY_SD_MD_CLIENT;
    this->source.state = RPY_SD_ST_UNSELECTED;
    // initialize polling
    this->source.maxPoll = PEER_MAX_POLL;
    this->source.minPoll = PEER_MIN_POLL;
    this->source.poll = PEER_MIN_POLL;
    this->pollNext = CLK_MONO();
}

void NtpPeer_recv(volatile void *pObj, uint8_t *frame, int flen) {
    // typecast "this" pointer
    NtpPeer *this = (NtpPeer *) pObj;
    ++this->source.rxCount;

    // map headers
    struct FRAME_ETH *headerEth = (struct FRAME_ETH *) frame;
    struct HEADER_IPv4 *headerIPv4 = (struct HEADER_IPv4 *) (headerEth + 1);
    struct HEADER_UDP *headerUDP = (struct HEADER_UDP *) (headerIPv4 + 1);
    struct HEADER_NTPv4 *headerNTP = (struct HEADER_NTPv4 *) (headerUDP + 1);

    // drop unsolicited packets
    if(headerNTP->origTime == 0)
        return;

    // check for interleaved response
    if(this->pollXleave) {
        // discard non-interleaved packets
        if(headerNTP->origTime == this->local_tx_hw[0]) {
            // packet received, but it was not interleaved
            this->pktRecv = true;
            this->source.xleave = false;
            return;
        }
        // discard unsolicited packets
        if(headerNTP->origTime != this->local_rx_hw[0])
            return;
        // replace remote TX timestamp with updated value
        this->local_tx = headerNTP->txTime;
        // packet received, and it was interleaved
        this->pktRecv = true;
        this->source.xleave = true;
        // increment count of valid packets
        ++this->source.rxValid;
        return;
    }

    // drop unsolicited packets
    if(headerNTP->origTime != this->local_tx)
        return;
    // prevent reception of duplicate packets
    this->local_tx = 0;
    // packet received
    this->pktRecv = true;
    // set hardware timestamp
    NET_getRxTime(frame, this->local_rx_hw);

    // set stratum
    this->source.stratum = headerNTP->stratum;
    this->source.leap = headerNTP->status;
    this->source.precision = headerNTP->precision;
    this->source.version = headerNTP->version;
    this->source.refID = headerNTP->refID;
    this->source.refTime = headerNTP->refTime;
    // set remote timestamps
    this->remote_rx = headerNTP->rxTime;
    this->remote_tx = headerNTP->txTime;
    // set root metrics
    this->source.rootDelay = __builtin_bswap32(headerNTP->rootDelay);
    this->source.rootDispersion = __builtin_bswap32(headerNTP->rootDispersion);
    // increment count of valid packets
    ++this->source.rxValid;
}
