//
// Created by robert on 5/21/22.
//

#include <memory.h>

#include "ntp.h"
#include "lib/clk.h"
#include "lib/net.h"
#include "lib/net/eth.h"
#include "lib/net/ip.h"
#include "lib/net/udp.h"
#include "lib/net/util.h"
#include "lib/led.h"
#include "gpsdo.h"
#include "hw/emac.h"
#include "hw/sys.h"
#include "lib/net/arp.h"
#include "lib/format.h"
#include "lib/net/dns.h"

#define NTP3_SIZE (UDP_DATA_OFFSET + 48)
#define NTP_PORT (123)
#define NTP_POOL_FQDN ("pool.ntp.org")

struct PACKED FRAME_NTPv3 {
    uint16_t mode: 3;
    uint16_t version: 3;
    uint16_t status: 2;
    uint8_t stratum;
    uint8_t poll;
    int8_t precision;
    uint32_t rootDelay;
    uint32_t rootDispersion;
    uint8_t refID[4];
    uint32_t refTime[2];
    uint32_t origTime[2];
    uint32_t rxTime[2];
    uint32_t txTime[2];
};
_Static_assert(sizeof(struct FRAME_NTPv3) == 48, "FRAME_NTPv3 must be 48 bytes");

static volatile uint32_t ntpEra = 0;
static volatile uint64_t ntpTimeOffset = 0;

#define SERVER_COUNT (8)
struct Server {
    uint64_t offset;
    uint32_t nextPoll;
    uint32_t lastResponse;
    uint32_t addr;
    uint8_t mac[6];
    uint8_t reach;
    uint8_t stratum;
} servers[SERVER_COUNT];

void processServerResponse(uint8_t *frame);

void NTP_process(uint8_t *frame, int flen);

void NTP_process0(const uint8_t *frame, struct FRAME_NTPv3 *frameNTP);
void NTP_followup0(uint8_t *frame, int flen, uint32_t txSec, uint32_t txNano);

void NTP_process3(const uint8_t *frame, struct FRAME_NTPv3 *frameNTP);

void NTP_init() {
    memset(servers, 0, sizeof(servers));
    UDP_register(NTP_PORT, NTP_process);
}

uint64_t NTP_offset() {
    return ntpTimeOffset;
}

void NTP_date(uint64_t clkMono, uint32_t *ntpDate) {
    uint64_t rollover = ((uint32_t *)&clkMono)[1];
    rollover += ((uint32_t *)&ntpTimeOffset)[1];
    clkMono += ntpTimeOffset;
    ntpDate[0] = ntpEra + ((uint32_t *)&rollover)[1];
    ntpDate[1] = __builtin_bswap32(((uint32_t *)&clkMono)[1]);
    ntpDate[2] = __builtin_bswap32(((uint32_t *)&clkMono)[0]);
    ntpDate[3] = 0;
}

void NTP_process(uint8_t *frame, int flen) {
    // discard malformed packets
    if(flen < 90) return;
    // map headers
    struct FRAME_ETH *headerEth = (struct FRAME_ETH *) frame;
    struct HEADER_IPv4 *headerIPv4 = (struct HEADER_IPv4 *) (headerEth + 1);
    struct HEADER_UDP *headerUDP = (struct HEADER_UDP *) (headerIPv4 + 1);
    struct FRAME_NTPv3 *frameNTP = (struct FRAME_NTPv3 *) (headerUDP + 1);

    // verify destination
    if(headerIPv4->dst != ipAddress) return;
    // filter non-client frames
    if(frameNTP->mode != 3) {
        if(frameNTP->mode == 4)
            processServerResponse(frame);
        return;
    }
    // time-server activity
    LED_act0();

    // modify ethernet frame header
    copyMAC(headerEth->macDst, headerEth->macSrc);
    // modify IP header
    headerIPv4->dst = headerIPv4->src;
    headerIPv4->src = ipAddress;
    // modify UDP header
    headerUDP->portDst = headerUDP->portSrc;
    headerUDP->portSrc = __builtin_bswap16(NTP_PORT);

    // get TX descriptor
    int txDesc = NET_getTxDesc();
    if(txDesc < 0) return;

    // process message
    if(frameNTP->version == 0) {
        NTP_process0(frame, frameNTP);
        NET_setTxCallback(txDesc, NTP_followup0);
    }
    else {
        NTP_process3(frame, frameNTP);
    }

    // finalize packet
    UDP_finalize(frame, flen);
    IPv4_finalize(frame, flen);
    // transmit packet
    memcpy(NET_getTxBuff(txDesc), frame, flen);
    NET_transmit(txDesc, flen);
}

void NTP_process0(const uint8_t *frame, struct FRAME_NTPv3 *frameNTP) {
    // retrieve rx time
    uint32_t rxTime[2];
    NET_getRxTimeRaw(frame, rxTime);

    // set type to server response
    frameNTP->mode = 4;
    // indicate that the time is not currently set
    frameNTP->status = GPSDO_isLocked() ? 0 : 3;
    // set other header fields
    frameNTP->stratum = GPSDO_isLocked() ? 1 : 16;
    frameNTP->precision = -24;
    // set reference ID
    memcpy(frameNTP->refID, "GPS", 4);
    // set root delay
    frameNTP->rootDelay = 0;
    // set root dispersion
    frameNTP->rootDispersion = 0;
    // set origin timestamp
    frameNTP->origTime[0] = frameNTP->txTime[0];
    frameNTP->origTime[1] = frameNTP->txTime[1];
    // set RX time
    frameNTP->rxTime[0] = ((uint32_t *) &rxTime)[0];
    frameNTP->rxTime[1] = ((uint32_t *) &rxTime)[1];
    // set reference time
    uint32_t refTime[2] = {EMAC0.TIMSEC, EMAC0.TIMNANO };
    if((int32_t)(refTime[1] - rxTime[1]) < 0) {
        if(refTime[0] == rxTime[0]) ++refTime[0];
    }
    frameNTP->refTime[0] = refTime[0];
    frameNTP->refTime[1] = refTime[1];
    // no TX time
    frameNTP->txTime[0] = 0;
    frameNTP->txTime[1] = 0;
}

void NTP_followup0(uint8_t *frame, int flen, uint32_t txSec, uint32_t txNano) {
    // discard malformed packets
    if(flen < 90) return;

    // map headers
    struct FRAME_ETH *headerEth = (struct FRAME_ETH *) frame;
    struct HEADER_IPv4 *headerIPv4 = (struct HEADER_IPv4 *) (headerEth + 1);
    struct HEADER_UDP *headerUDP = (struct HEADER_UDP *) (headerIPv4 + 1);
    struct FRAME_NTPv3 *frameNTP = (struct FRAME_NTPv3 *) (headerUDP + 1);

    // guard against buffer overruns
    if(headerEth->ethType != ETHTYPE_IPv4) return;
    if(headerIPv4->proto != IP_PROTO_UDP) return;
    if(headerUDP->portSrc != __builtin_bswap16(NTP_PORT)) return;
    if(frameNTP->version != 0) return;

    // append original TX time and resend
    frameNTP->txTime[0] = txSec;
    frameNTP->txTime[1] = txNano;

    // send packet
    UDP_finalize(frame, flen);
    IPv4_finalize(frame, flen);
    // get TX descriptor
    int txDesc = NET_getTxDesc();
    if(txDesc < 0) return;
    // get new TX buffer
    uint8_t *newFrame = NET_getTxBuff(txDesc);
    // copy frame to new TX buffer (but only if the new buffer has a different address)
    if(newFrame != frame)
        memcpy(newFrame, frame, flen);
    // transmit response
    NET_transmit(txDesc, flen);
}

void NTP_process3(const uint8_t *frame, struct FRAME_NTPv3 *frameNTP) {
    // retrieve rx time
    uint64_t rxTime = NET_getRxTime(frame) + ntpTimeOffset;

    // set type to server response
    frameNTP->mode = 4;
    // indicate that the time is not currently set
    frameNTP->status = GPSDO_isLocked() ? 0 : 3;
    // set other header fields
    frameNTP->stratum = GPSDO_isLocked() ? 1 : 16;
    frameNTP->precision = -24;
    // set reference ID
    memcpy(frameNTP->refID, "GPS", 4);
    // set root delay
    frameNTP->rootDelay = 0;
    // set root dispersion
    frameNTP->rootDispersion = 0;
    // set origin timestamp
    frameNTP->origTime[0] = frameNTP->txTime[0];
    frameNTP->origTime[1] = frameNTP->txTime[1];
    // set reference timestamp
    uint64_t refTime = CLK_TAI() + ntpTimeOffset;
    frameNTP->refTime[0] = __builtin_bswap32(((uint32_t *) &refTime)[1]);
    frameNTP->refTime[1] = __builtin_bswap32(((uint32_t *) &refTime)[0]);
    // set RX time
    frameNTP->rxTime[0] = __builtin_bswap32(((uint32_t *) &rxTime)[1]);
    frameNTP->rxTime[1] = __builtin_bswap32(((uint32_t *) &rxTime)[0]);
    // set TX time
    uint64_t txTime = CLK_TAI() + ntpTimeOffset;
    frameNTP->txTime[0] = __builtin_bswap32(((uint32_t *) &txTime)[1]);
    frameNTP->txTime[1] = __builtin_bswap32(((uint32_t *) &txTime)[0]);
}

void poll3(uint8_t *frame, uint32_t addr, uint8_t *mac) {
    // clear frame buffer
    memset(frame, 0, NTP3_SIZE);

    // map headers
    struct FRAME_ETH *headerEth = (struct FRAME_ETH *) frame;
    struct HEADER_IPv4 *headerIPv4 = (struct HEADER_IPv4 *) (headerEth + 1);
    struct HEADER_UDP *headerUDP = (struct HEADER_UDP *) (headerIPv4 + 1);
    struct FRAME_NTPv3 *frameNTP = (struct FRAME_NTPv3 *) (headerUDP + 1);

    // EtherType = IPv4
    headerEth->ethType = ETHTYPE_IPv4;
    // MAC address
    copyMAC(headerEth->macDst, mac);

    // IPv4 Header
    IPv4_init(frame);
    headerIPv4->dst = addr;
    headerIPv4->src = ipAddress;
    headerIPv4->proto = IP_PROTO_UDP;

    // UDP Header
    headerUDP->portSrc = __builtin_bswap16(NTP_PORT);
    headerUDP->portDst = __builtin_bswap16(NTP_PORT);

    // set type to client request
    frameNTP->version = 3;
    frameNTP->mode = 3;
    // set reference ID
    memcpy(frameNTP->refID, "GPS", 4);
    // set TX time
    uint64_t txTime = CLK_TAI();
    frameNTP->txTime[0] = __builtin_bswap32(((uint32_t *) &txTime)[1]);
    frameNTP->txTime[1] = __builtin_bswap32(((uint32_t *) &txTime)[0]);
}

void pollServer(int index) {
    int txDesc = NET_getTxDesc();
    if(txDesc < 0) return;
    uint8_t *frame = NET_getTxBuff(txDesc);

    // create NTP request
    poll3(frame, servers[index].addr, servers[index].mac);

    // transmit request
    UDP_finalize(frame, NTP3_SIZE);
    IPv4_finalize(frame, NTP3_SIZE);
    NET_transmit(txDesc, NTP3_SIZE);
    // advance reach window
    servers[index].reach <<= 1;
}

static void arpCallback(uint32_t remoteAddress, uint8_t *macAddress) {
    // special case for external addresses
    if(remoteAddress == ipGateway) {
        for(int i = 0; i < SERVER_COUNT; i++) {
            if((servers[i].addr ^ ipAddress) & ipSubnet) {
                copyMAC(servers[i].mac, macAddress);
            }
        }
        return;
    }

    for(int i = 0; i < SERVER_COUNT; i++) {
        if(servers[i].addr == remoteAddress) {
            copyMAC(servers[i].mac, macAddress);
            break;
        }
    }
}

void runServer(int index, uint32_t now) {
    // poll client if configured
    if(servers[index].addr == 0)
        return;

    // perform ARP request if MAC address is missing
    if(isNullMAC(servers[index].mac)) {
        // determine if server is outside of current subnet
        if((servers[index].addr ^ ipAddress) & ipSubnet)
            ARP_request(ipGateway, arpCallback);
        else
            ARP_request(servers[index].addr, arpCallback);
        return;
    }

    // verify current time
    uint32_t nextPoll = servers[index].nextPoll;
    if(((int32_t) (now - nextPoll)) < 0)
        return;

    // schedule next poll
    nextPoll &= ~63;
    nextPoll += 64;
    nextPoll += (STCURRENT.CURRENT >> 8) & 7;
    servers[index].nextPoll = nextPoll;

    // send poll request
    pollServer(index);
}

static void dnsCallback(uint32_t addr) {
    // prevent duplicate entries
    for (int i = 0; i < SERVER_COUNT; i++) {
        if(servers[i].addr == addr)
            return;
    }
    // fill empty server slot
    for (int i = 0; i < SERVER_COUNT; i++) {
        if (servers[i].addr == 0) {
            servers[i].addr = addr;
            servers[i].nextPoll = CLK_MONOTONIC_INT() + 2;
            break;
        }
    }
}

static uint32_t nextPoll = 0;
void NTP_run() {
    uint32_t now = CLK_MONOTONIC_INT();
    if(((int32_t) (now - nextPoll)) < 0) return;
    nextPoll = now + 1;

    // search for additional NTP servers
    int empty = 0;
    for(int i = 0; i < SERVER_COUNT; i++)
        if(servers[i].addr == 0) ++empty;
    if(empty > 0)
        DNS_lookup(NTP_POOL_FQDN, dnsCallback);

    // process existing servers
    for(int i = 0; i < SERVER_COUNT; i++)
        runServer(i, now);

    int64_t diff = (int64_t) (servers[0].offset - ntpTimeOffset);
    diff >>= 31;
    if(diff != 0)
        ((uint32_t *) &ntpTimeOffset)[1] += diff >> 1;
}

char* NTP_servers(char *tail) {
    char tmp[32];

    uint32_t now = CLK_MONOTONIC_INT();
    for(int i = 0; i < SERVER_COUNT; i++) {
        if(servers[i].addr == 0) continue;

        tail = append(tail, "  - [");
        tail = macToStr(servers[i].mac, tail);
        tail = append(tail, "] ");
        tail = addrToStr(servers[i].addr, tail);
        tail = append(tail, "\n");

        tail = append(tail, "    ");
        tmp[toDec(servers[i].stratum, 3, ' ', tmp)] = 0;
        tail = append(tail, tmp);
        tail = append(tail, " ");
        tmp[toDec(servers[i].nextPoll - now, 3, ' ', tmp)] = 0;
        tail = append(tail, tmp);
        tail = append(tail, " ");
        tmp[toDec(now - servers[i].lastResponse, 3, ' ', tmp)] = 0;
        tail = append(tail, tmp);
        tail = append(tail, " ");
        tmp[toOct(servers[i].reach, 3, '0', tmp)] = 0;
        tail = append(tail, tmp);

        strcpy(tmp, "  0x");
        toHex(servers[i].offset>>32, 8, '0', tmp+4);
        tmp[12] = '.';
        toHex(servers[i].offset, 8, '0', tmp+13);
        tmp[21] = 0;
        tail = append(tail, tmp);
        tail = append(tail, "\n");
    }
    return tail;
}

void processServerResponse(uint8_t *frame) {
    uint64_t rxTime = CLK_TAI();

    // map headers
    struct FRAME_ETH *headerEth = (struct FRAME_ETH *) frame;
    struct HEADER_IPv4 *headerIPv4 = (struct HEADER_IPv4 *) (headerEth + 1);
    struct HEADER_UDP *headerUDP = (struct HEADER_UDP *) (headerIPv4 + 1);
    struct FRAME_NTPv3 *frameNTP = (struct FRAME_NTPv3 *) (headerUDP + 1);

    uint64_t origTime, recvTime, txTime;
    ((uint32_t *) &origTime)[1] = __builtin_bswap32(frameNTP->origTime[0]);
    ((uint32_t *) &origTime)[0] = __builtin_bswap32(frameNTP->origTime[1]);

    ((uint32_t *) &recvTime)[1] = __builtin_bswap32(frameNTP->rxTime[0]);
    ((uint32_t *) &recvTime)[0] = __builtin_bswap32(frameNTP->rxTime[1]);

    ((uint32_t *) &txTime)[1] = __builtin_bswap32(frameNTP->txTime[0]);
    ((uint32_t *) &txTime)[0] = __builtin_bswap32(frameNTP->txTime[1]);

    uint64_t a = recvTime - origTime;
    uint64_t b = txTime - rxTime;
    int64_t diff = b - a;
    diff >>= 1;
    a += diff;

    for(int i = 0; i < SERVER_COUNT; i++) {
        if(servers[i].addr == headerIPv4->src) {
            servers[i].reach |= 1;
            servers[i].lastResponse = CLK_MONOTONIC_INT();
            servers[i].offset = a;
            servers[i].stratum = frameNTP->stratum;
            break;
        }
    }
}
