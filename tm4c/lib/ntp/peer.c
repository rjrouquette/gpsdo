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
}

static void runPoll(NtpPeer *this) {
    this->source.reach <<= 1;
    if(!this->pktSent) {
        this->pktSent = true;
        sendPoll(this);
        return;
    }
    if(this->pktRecv) {
        this->pollActive = false;
    } else {
        uint64_t now = CLK_MONO();
        if((now - this->pollStart) > PEER_RESPONSE_TIMEOUT) {
            this->pollActive = false;
        }
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

    // set reach bit
    this->source.reach |= 1;
    // set response time
    this->source.lastResponse = CLK_MONO_INT();
    // set stratum
    this->source.stratum = headerNTP->stratum;
    // set hardware timestamp
    NET_getRxTime(frame, this->local_rx_hw);
    // signal packet received
    this->pktRecv = true;
}
