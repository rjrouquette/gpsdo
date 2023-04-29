//
// Created by robert on 4/27/23.
//

#include <memory.h>
#include "../chrony/candm.h"
#include "../clk/mono.h"
#include "../led.h"
#include "../rand.h"
#include "../net.h"
#include "../net/arp.h"
#include "../net/eth.h"
#include "../net/util.h"
#include "common.h"
#include "peer.h"
#include "../clk/tai.h"


#define ARP_MIN_AGE (4) // maximum polling rate
#define ARP_MAX_AGE (64) // refresh old MAC addresses

static uint64_t avg64(uint64_t a, uint64_t b) {
    int64_t diff = (int64_t) (b - a);
    return a + (diff >> 1);
}

// ARP response callback
static void arpCallback(void *ref, uint32_t remoteAddr, uint8_t *macAddr) {
    // typecast "this" pointer
    NtpPeer *this = (struct NtpPeer *) ref;
    // update MAC address
    copyMAC((uint8_t *) this->macAddr, macAddr);
}

// Checks MAC address of peer and performs ARP request to retrieve it
static int checkMac(NtpPeer *this) {
    const uint32_t now = CLK_MONO_INT();
    const uint32_t age = now - this->lastArp;
    const int invalid = isNullMAC(this->macAddr) ? 1 : 0;
    if(invalid || (age > ARP_MAX_AGE)) {
        if (age > ARP_MIN_AGE) {
            this->lastArp = now;
            if(IPv4_testSubnet(ipSubnet, ipAddress, this->source.id))
                ARP_request(ipGateway, arpCallback, this);
            else
                ARP_request(this->source.id, arpCallback, this);
        }
    }
    return invalid;
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
    headerNTP->stratum = 0;//clockStratum;
    headerNTP->precision = -32;//NTP_;
    // set reference ID
    headerNTP->refID = 0;//refId;
    // set reference timestamp
    headerNTP->refTime = 0;//__builtin_bswap64(ntpModified());
    // set origin timestamp
    headerNTP->origTime = 0;//server->prevTxSrv;
    // set rx time timestamp
    headerNTP->rxTime = 0;//server->prevRx;
    // set TX time
    uint64_t txTime = (CLK_TAI() - clkTaiUtcOffset) + NTP_UTC_OFFSET;
    headerNTP->txTime = __builtin_bswap64(txTime);

    this->local_tx = txTime;
    NET_setTxCallback(txDesc, txCallback, this);

    // transmit request
    UDP_finalize(frame, NTP4_SIZE);
    IPv4_finalize(frame, NTP4_SIZE);
    NET_transmit(txDesc, NTP4_SIZE);
}

static void startPoll(NtpPeer *this) {
    const uint64_t now = CLK_MONO();
    // schedule next poll
    uint64_t nextOffset = RAND_next();
    nextOffset >>= this->source.poll - 4;
    nextOffset += 1ull << (this->source.poll + 32);
    this->nextPoll += nextOffset;
    // start poll
    this->pollStart = now;
    this->pollActive = true;
    this->pollBurst = 0xFF;
    this->pollRetry = 0;
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
        sendPoll(this);
        return;
    }

    // receive packet
    if(this->pktRecv) {
        // get pointer to current burst sample
        NtpPollSample *burstSample = this->burstSamples + this->pollBurst;
        // set monotonic reference time
//        burstSample->offset.mono = (int64_t) avg64(this->local_tx_hw[0], this->local_rx_hw[0]);
        burstSample->offset.mono = (int64_t) this->local_tx_hw[0];
        // set compensated reference time
//        burstSample->offset.comp = (int64_t) avg64(this->local_tx_hw[1], this->local_rx_hw[1]);
        burstSample->offset.comp = (int64_t) this->local_tx_hw[1];
        // compute delay (use compensated clock for best accuracy)
        int64_t delay = (int64_t) avg64(this->remote_rx - this->local_tx_hw[1], this->local_rx_hw[1] - this->remote_tx);
        burstSample->delay = 0x1p-32f * (float) (int32_t) delay;
        // compute TAI offset
//        uint64_t offset = avg64(this->remote_rx - this->local_tx_hw[2], this->remote_tx - this->local_rx_hw[2]);
        uint64_t offset = this->remote_rx - this->local_tx_hw[2];
        burstSample->offset.tai = (int64_t) ((offset - NTP_UTC_OFFSET) + clkTaiUtcOffset);

        if(++this->pollBurst >= PEER_BURST_SIZE) {
            // burst complete
            this->pollActive = false;
        } else {
            // perform next poll in burst
            this->pktSent = false;
        }
    } else {
        uint64_t now = CLK_MONO();
        if((now - this->pollStart) > PEER_RESPONSE_TIMEOUT) {
            // check if reattempt is allowed
            if(++this->pollRetry > PEER_BURST_RETRIES) {
                // retry count exceeded
                this->pollActive = false;
            } else {
                // retry poll
                this->pktSent = false;
            }
        }
    }

    // end of burst
    if(!this->pollActive) {
        // no burst samples completed
        if(this->pollBurst < 1) return;
        // promote a burst sample as the final sample

        // advance sample buffer
        NtpSource_incr(&this->source);
        // set current sample
        NtpPollSample *sample = this->source.pollSample + this->source.samplePtr;
        *sample = this->burstSamples[0];
        // update filter
        NtpSource_update(&this->source);
    }
}

static void run(volatile void *pObj) {
    // typecast "this" pointer
    NtpPeer *this = (NtpPeer *) pObj;

    // check MAC address
    if(checkMac(this)) return;

    // check for start of poll
    const uint64_t now = CLK_MONO();
    if(((int64_t)(now - this->nextPoll)) > 0)
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
    this->source.poll = PEER_MIN_POLL;
    this->nextPoll = CLK_MONO();
}

void NtpPeer_recv(volatile void *pObj, uint8_t *frame, int flen) {
    // typecast "this" pointer
    NtpPeer *this = (NtpPeer *) pObj;

    // map headers
    struct FRAME_ETH *headerEth = (struct FRAME_ETH *) frame;
    struct HEADER_IPv4 *headerIPv4 = (struct HEADER_IPv4 *) (headerEth + 1);
    struct HEADER_UDP *headerUDP = (struct HEADER_UDP *) (headerIPv4 + 1);
    struct HEADER_NTPv4 *headerNTP = (struct HEADER_NTPv4 *) (headerUDP + 1);

    // drop unsolicited packets
    if(headerNTP->origTime != __builtin_bswap64(this->local_tx))
        return;
    // prevent reception of duplicate packets
    this->local_tx = 0;

    // set reach bit
    this->source.reach |= 1;
    // set stratum
    this->source.stratum = headerNTP->stratum;
    // set remote timestamps
    this->remote_rx = __builtin_bswap64(headerNTP->rxTime);
    this->remote_tx = __builtin_bswap64(headerNTP->txTime);
    // set hardware timestamp
    NET_getRxTime(frame, this->local_rx_hw);
    // set response time
    this->source.lastResponse = CLK_MONO_INT();
    // signal packet received
    this->pktRecv = true;
}
