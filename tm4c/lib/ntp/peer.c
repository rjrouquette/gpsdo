//
// Created by robert on 4/27/23.
//

#include <memory.h>
#include <math.h>
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
#include "../clk/util.h"


#define ARP_MIN_AGE (4) // maximum polling rate
#define ARP_MAX_AGE (64) // refresh old MAC addresses


extern uint64_t tsDebug[3];


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
    headerNTP->origTime = this->remote_tx;
    // set rx time timestamp
    headerNTP->rxTime = __builtin_bswap64((this->local_rx_hw[2] - clkTaiUtcOffset) + NTP_UTC_OFFSET);
    // set TX time
    uint64_t txTime = __builtin_bswap64((CLK_TAI() - clkTaiUtcOffset) + NTP_UTC_OFFSET);
    headerNTP->txTime = txTime;
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
        this->pktRecv = false;
        sendPoll(this);
        return;
    }

    // receive packet
    if(this->pktRecv) {
        this->pktRecv = false;
        // translate remote timestamps
        uint64_t remote_rx = __builtin_bswap64(this->remote_rx);
        uint64_t remote_tx = __builtin_bswap64(this->remote_tx);
        // remove NTP offset
        ((uint32_t *) &remote_rx)[1] -= NTP_UTC_OFFSET;
        ((uint32_t *) &remote_tx)[1] -= NTP_UTC_OFFSET;
        // add TAI offset
        remote_rx += clkTaiUtcOffset;
        remote_tx += clkTaiUtcOffset;

        // get pointer to current burst sample
        NtpPollSample *burstSample = this->burstSamples + this->pollBurst;
        // set compensated reference time
        burstSample->comp = avg64(this->local_tx_hw[1], this->local_rx_hw[1]);
        // set TAI reference time
        burstSample->tai = avg64(this->local_tx_hw[2], this->local_rx_hw[2]);
        // compute TAI offset
        uint64_t offset = avg64(remote_rx, remote_tx) - burstSample->tai;
        burstSample->offset = (int64_t) offset;
        // compute delay (use compensated clock for best accuracy)
        int64_t delay = (int64_t) ((this->local_tx_hw[1] - this->local_rx_hw[1]) - (remote_rx - remote_tx));
        burstSample->delay = 0x1p-31f * (float) (int32_t) (delay >> 2);

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
        const int count = this->pollBurst;
        // no burst samples completed
        if(count < 1) return;
        // promote a burst sample as the final sample
        int selected = 0;
        // special case for exactly two samples
        if(count == 2) {
            if(this->burstSamples[1].delay < this->burstSamples[0].delay)
                selected = 1;
        }
        // select sample closest to the mean
        else if(count > 2) {
            float offset[count];
            float mean = 0;
            for(int i = 0; i < count; i++) {
                offset[i] = toFloat(this->burstSamples[i].offset);
                mean += offset[i];
            }
            float best = fabsf(offset[0] - mean);
            for(int i = 0; i < count; i++) {
                float dist = fabsf(offset[0] - mean);
                if(dist < best) {
                    best = dist;
                    selected = i;
                }
            }
        }

        // advance sample buffer
        NtpSource_incr(&this->source);
        // set current sample
        NtpPollSample *sample = this->source.pollSample + this->source.samplePtr;
        *sample = this->burstSamples[selected];
        // update filter
        NtpSource_update(&this->source);
        // set update time
        this->source.lastUpdate = CLK_MONO();
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
    if(headerNTP->origTime != this->local_tx)
        return;
    // indicate packet received
    this->pktRecv = true;
    // prevent reception of duplicate packets
    this->local_tx = 0;
    // set hardware timestamp
    NET_getRxTime(frame, this->local_rx_hw);

    // set reach bit
    this->source.reach |= 1;
    // set stratum
    this->source.stratum = headerNTP->stratum;
    // set remote timestamps
    this->remote_rx = headerNTP->rxTime;
    this->remote_tx = headerNTP->txTime;
    // set root metrics
    this->source.rootDelay = __builtin_bswap32(headerNTP->rootDelay);
    this->source.rootDispersion = __builtin_bswap32(headerNTP->rootDispersion);
}
