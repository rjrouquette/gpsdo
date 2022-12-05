//
// Created by robert on 5/21/22.
//

#include <memory.h>
#include <math.h>

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

#define NTP4_SIZE (UDP_DATA_OFFSET + 48)
#define NTP_PORT (123)
#define NTP_BURST (4)
#define NTP_BURST_BITS (2)
#define NTP_POOL_FQDN ("pool.ntp.org")

struct PACKED HEADER_NTPv4 {
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
_Static_assert(sizeof(struct HEADER_NTPv4) == 48, "HEADER_NTPv4 must be 48 bytes");

static volatile uint32_t ntpEra = 0;
static volatile uint64_t ntpTimeOffset = 0;

#define SERVER_COUNT (8)
struct Server {
    float delay;
    float jitter;
    float drift;
    float variance;
    uint64_t stamps[NTP_BURST << 2];
    uint64_t offset;
    uint64_t update;
    uint32_t nextPoll;
    uint32_t lastResponse;
    uint32_t attempts;
    uint32_t addr;
    uint8_t mac[6];
    uint8_t reach;
    uint8_t stratum;
    uint8_t burst;
} servers[SERVER_COUNT];

static void processResponse(uint8_t *frame);
static void pollServer(struct Server *server, int pingOnly);

static void processRequest(uint8_t *frame, int flen);
static void processRequest0(const uint8_t *frame, struct HEADER_NTPv4 *headerNTP);
static void followupRequeust0(uint8_t *frame, int flen, uint32_t txSec, uint32_t txNano);
static void processRequest4(const uint8_t *frame, struct HEADER_NTPv4 *headerNTP);

void NTP_init() {
    memset(servers, 0, sizeof(servers));
    UDP_register(NTP_PORT, processRequest);
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

void processRequest(uint8_t *frame, int flen) {
    // discard malformed packets
    if(flen < 90) return;
    // map headers
    struct FRAME_ETH *headerEth = (struct FRAME_ETH *) frame;
    struct HEADER_IPv4 *headerIPv4 = (struct HEADER_IPv4 *) (headerEth + 1);
    struct HEADER_UDP *headerUDP = (struct HEADER_UDP *) (headerIPv4 + 1);
    struct HEADER_NTPv4 *headerNTP = (struct HEADER_NTPv4 *) (headerUDP + 1);

    // verify destination
    if(headerIPv4->dst != ipAddress) return;
    // filter non-client frames
    if(headerNTP->mode != 3) {
        if(headerNTP->mode == 4)
            processResponse(frame);
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
    if(headerNTP->version == 0) {
        processRequest0(frame, headerNTP);
        NET_setTxCallback(txDesc, followupRequeust0);
    }
    else {
        processRequest4(frame, headerNTP);
    }

    // finalize packet
    UDP_finalize(frame, flen);
    IPv4_finalize(frame, flen);
    // transmit packet
    memcpy(NET_getTxBuff(txDesc), frame, flen);
    NET_transmit(txDesc, flen);
}

static void processRequest0(const uint8_t *frame, struct HEADER_NTPv4 *headerNTP) {
    // retrieve rx time
    uint32_t rxTime[2];
    NET_getRxTimeRaw(frame, rxTime);

    // set type to server response
    headerNTP->mode = 4;
    // indicate that the time is not currently set
    headerNTP->status = GPSDO_isLocked() ? 0 : 3;
    // set other header fields
    headerNTP->stratum = GPSDO_isLocked() ? 1 : 16;
    headerNTP->precision = -24;
    // set reference ID
    memcpy(headerNTP->refID, "GPS", 4);
    // set root delay
    headerNTP->rootDelay = 0;
    // set root dispersion
    headerNTP->rootDispersion = 0;
    // set origin timestamp
    headerNTP->origTime[0] = headerNTP->txTime[0];
    headerNTP->origTime[1] = headerNTP->txTime[1];
    // set RX time
    headerNTP->rxTime[0] = ((uint32_t *) &rxTime)[0];
    headerNTP->rxTime[1] = ((uint32_t *) &rxTime)[1];
    // set reference time
    uint32_t refTime[2] = {EMAC0.TIMSEC, EMAC0.TIMNANO };
    if((int32_t)(refTime[1] - rxTime[1]) < 0) {
        if(refTime[0] == rxTime[0]) ++refTime[0];
    }
    headerNTP->refTime[0] = refTime[0];
    headerNTP->refTime[1] = refTime[1];
    // no TX time
    headerNTP->txTime[0] = 0;
    headerNTP->txTime[1] = 0;
}

static void followupRequeust0(uint8_t *frame, int flen, uint32_t txSec, uint32_t txNano) {
    // discard malformed packets
    if(flen < 90) return;

    // map headers
    struct FRAME_ETH *headerEth = (struct FRAME_ETH *) frame;
    struct HEADER_IPv4 *headerIPv4 = (struct HEADER_IPv4 *) (headerEth + 1);
    struct HEADER_UDP *headerUDP = (struct HEADER_UDP *) (headerIPv4 + 1);
    struct HEADER_NTPv4 *headerNTP = (struct HEADER_NTPv4 *) (headerUDP + 1);

    // guard against buffer overruns
    if(headerEth->ethType != ETHTYPE_IPv4) return;
    if(headerIPv4->proto != IP_PROTO_UDP) return;
    if(headerUDP->portSrc != __builtin_bswap16(NTP_PORT)) return;
    if(headerNTP->version != 0) return;

    // append original TX time and resend
    headerNTP->txTime[0] = txSec;
    headerNTP->txTime[1] = txNano;

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

static void processRequest4(const uint8_t *frame, struct HEADER_NTPv4 *headerNTP) {
    // retrieve rx time
    uint64_t rxTime = NET_getRxTime(frame) + ntpTimeOffset;

    // set type to server response
    headerNTP->mode = 4;
    // indicate that the time is not currently set
    headerNTP->status = GPSDO_isLocked() ? 0 : 3;
    // set other header fields
    headerNTP->stratum = GPSDO_isLocked() ? 1 : 16;
    headerNTP->precision = -24;
    // set reference ID
    memcpy(headerNTP->refID, "GPS", 4);
    // set root delay
    headerNTP->rootDelay = 0;
    // set root dispersion
    headerNTP->rootDispersion = 0;
    // set origin timestamp
    headerNTP->origTime[0] = headerNTP->txTime[0];
    headerNTP->origTime[1] = headerNTP->txTime[1];
    // set reference timestamp
    uint64_t refTime = CLK_TAI() + ntpTimeOffset;
    headerNTP->refTime[0] = __builtin_bswap32(((uint32_t *) &refTime)[1]);
    headerNTP->refTime[1] = __builtin_bswap32(((uint32_t *) &refTime)[0]);
    // set RX time
    headerNTP->rxTime[0] = __builtin_bswap32(((uint32_t *) &rxTime)[1]);
    headerNTP->rxTime[1] = __builtin_bswap32(((uint32_t *) &rxTime)[0]);
    // set TX time
    uint64_t txTime = CLK_TAI() + ntpTimeOffset;
    headerNTP->txTime[0] = __builtin_bswap32(((uint32_t *) &txTime)[1]);
    headerNTP->txTime[1] = __builtin_bswap32(((uint32_t *) &txTime)[0]);
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

static void runServer(struct Server *server, uint32_t now) {
    // poll client if configured
    if(server->addr == 0)
        return;

    // perform ARP request if MAC address is missing
    if(isNullMAC(servers->mac)) {
        // determine if server is outside of current subnet
        if((servers->addr ^ ipAddress) & ipSubnet)
            ARP_request(ipGateway, arpCallback);
        else
            ARP_request(servers->addr, arpCallback);
        return;
    }

    // clear entry if the server is unreachable
    if(server->reach == 0 && servers->attempts > 8) {
        memset(server, 0, sizeof(struct Server));
        return;
    }

    // verify current time
    uint32_t nextPoll = server->nextPoll;
    if(((int32_t) (now - nextPoll)) < -1)
        return;
    if(((int32_t) (now - nextPoll)) == -1) {
        // ping server in advance of actual poll request (reduces jitter)
        pollServer(server, 1);
        return;
    }

    // schedule next poll
    nextPoll &= ~63;
    nextPoll += 64;
    nextPoll += (STCURRENT.CURRENT >> 8) & 7;
    servers->nextPoll = nextPoll;

    // advance reach window
    server->reach <<= 1;
    ++(server->attempts);
    // send poll request
    server->burst = 0;
    memset(server->stamps, 0, sizeof(server->stamps));
    pollServer(server, 0);
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
            servers[i].lastResponse = CLK_MONOTONIC_INT();
            servers[i].nextPoll = servers[i].lastResponse + 2;
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
        runServer(servers + i, now);

    int64_t diff = (int64_t) (servers[0].offset - ntpTimeOffset);
    diff >>= 31;
    if(diff != 0)
        ((uint32_t *) &ntpTimeOffset)[1] += diff >> 1;
}

char* NTP_servers(char *tail) {
    char tmp[32];

    tail = append(tail, "ntp servers: (");
    tail = append(tail, NTP_POOL_FQDN);
    tail = append(tail, ")\n");

    uint32_t now = CLK_MONOTONIC_INT();
    for(int i = 0; i < SERVER_COUNT; i++) {
        if(servers[i].addr == 0) continue;

        tail = append(tail, "  - ");
        char *pad = tail + 16;
        tail = addrToStr(servers[i].addr, tail);
        while(tail < pad)
            *(tail++) = ' ';

        tmp[toDec(servers[i].stratum, 3, ' ', tmp)] = 0;
        tail = append(tail, tmp);
        tail = append(tail, " ");
        tmp[toOct(servers[i].reach, 3, '0', tmp)] = 0;
        tail = append(tail, tmp);
        tail = append(tail, " ");
        tmp[toDec(servers[i].nextPoll - now, 3, ' ', tmp)] = 0;
        tail = append(tail, tmp);
        tail = append(tail, "s ");
        tmp[toDec(now - servers[i].lastResponse, 3, ' ', tmp)] = 0;
        tail = append(tail, tmp);
        tail = append(tail, "s ");

        strcpy(tmp, "0x");
        toHex(servers[i].offset>>32, 8, '0', tmp+2);
        tmp[10] = '.';
        toHex(servers[i].offset, 8, '0', tmp+11);
        tmp[19] = 0;
        tail = append(tail, tmp);

        tail = append(tail, " ");
        tmp[fmtFloat(servers[i].delay * 1e3f, 9, 3, tmp)] = 0;
        tail = append(tail, tmp);

        tail = append(tail, "ms ");
        tmp[fmtFloat(sqrtf(servers[i].jitter) * 1e3f, 9, 3, tmp)] = 0;
        tail = append(tail, tmp);

        tail = append(tail, "ms ");
        tmp[fmtFloat(servers[i].drift * 1e6f, 9, 3, tmp)] = 0;
        tail = append(tail, tmp);

        tail = append(tail, "ppm ");
        tmp[fmtFloat(sqrtf(servers[i].variance) * 1e6f, 9, 3, tmp)] = 0;
        tail = append(tail, tmp);

        tail = append(tail, "ppm\n");
    }
    return tail;
}

static void updateTime(struct Server *server) {
    // compute delay and offset
    int64_t _adjust = 0;
    int64_t _delay = 0;
    uint64_t offset = server->stamps[1] - server->stamps[0];
    for(int i = 0; i < NTP_BURST; i++) {
        uint64_t *set = server->stamps + (i << 2);
        // accumulate delay
        _delay += (int64_t) ((set[4] - set[3]) - (set[2] - set[1]));
        // accumulate offset adjustment
        _adjust += ((int64_t) ((set[1] - set[0]) - offset));
        _adjust += ((int64_t) ((set[2] - set[3]) - offset));
    }
    // normalize delay
    _delay >>= NTP_BURST_BITS + 1;
    // adjust offset
    offset += _adjust >> (NTP_BURST_BITS + 1);

    // compute update time
    uint64_t update = server->stamps[(NTP_BURST << 2) - 1];
    update += (((int64_t) (server->stamps[0] - server->stamps[(NTP_BURST << 2) - 1])) >> 1);

    // update delay stats
    float delay = 0x1p-32f * (float) (int32_t) _delay;
    float jitter = delay - server->delay;
    jitter *= jitter;
    server->delay += (delay - server->delay) * 0x1p-4f;
    server->jitter += (jitter - server->jitter) * 0x1p-4f;

    // compute drift
    if(server->update != 0) {
        int64_t interval = (int64_t) (update - server->update);
        int32_t ipart = ((int32_t *) &interval)[1];
        if(ipart < 0) ipart = -ipart;
        int shift = 0;
        while((ipart >> shift) != 0)
            ++shift;
        float drift = (float) (int32_t) (offset - server->offset);
        drift /= (float) (int32_t) (interval >> shift);
        drift /= (float) (1 << shift);
        // update drift metrics
        if (fabsf(drift) < 1e-2f) {
            float var = drift - server->drift;
            var *= var;
            server->drift += (drift - server->drift) * 0x1p-4f;
            server->variance += (var - server->variance) * 0x1p-4f;
        }
    }

    // update server status
    server->offset = offset;
    server->update = update;
}

static void processResponse(uint8_t *frame) {
    // map headers
    struct FRAME_ETH *headerEth = (struct FRAME_ETH *) frame;
    struct HEADER_IPv4 *headerIPv4 = (struct HEADER_IPv4 *) (headerEth + 1);
    struct HEADER_UDP *headerUDP = (struct HEADER_UDP *) (headerIPv4 + 1);
    struct HEADER_NTPv4 *headerNTP = (struct HEADER_NTPv4 *) (headerUDP + 1);

    // validate server address
    struct Server *server = NULL;
    for(int i = 0; i < SERVER_COUNT; i++) {
        if(servers[i].addr == headerIPv4->src) {
            server = servers + 1;
            break;
        }
    }
    // ignore unsolicited packets
    if(server == NULL) return;
    if(server->burst >= NTP_BURST) return;
    // map timestamps
    uint64_t *stamps = server->stamps + (server->burst << 2);
    // record client RX timestamp
    stamps[3] = CLK_MONOTONIC();

    // verify client TX timestamp
    uint64_t txTime;
    ((uint32_t *)&txTime)[1] = __builtin_bswap32(headerNTP->origTime[0]);
    ((uint32_t *)&txTime)[0] = __builtin_bswap32(headerNTP->origTime[1]);
    // discard ping responses
    if(txTime == 0) return;
    // discard stale or duplicate packets
    if(txTime != stamps[0]) return;

    // copy server TX timestamp
    ((uint32_t *)(stamps + 1))[1] = __builtin_bswap32(headerNTP->rxTime[0]);
    ((uint32_t *)(stamps + 1))[0] = __builtin_bswap32(headerNTP->rxTime[1]);
    // copy server TX timestamp
    ((uint32_t *)(stamps + 2))[1] = __builtin_bswap32(headerNTP->txTime[0]);
    ((uint32_t *)(stamps + 2))[0] = __builtin_bswap32(headerNTP->txTime[1]);

    // discard responses with aberrant round-trip times
    int64_t roundTrip = (int64_t) (stamps[3] - stamps[0]);
    if(((uint32_t *) &roundTrip)[1] != 0) return;
    // discard responses with aberrant delays
    int64_t delay = ((int64_t) (roundTrip - (stamps[2] - stamps[1]))) >> 1;
    if(((uint32_t *) &delay)[1] != 0) return;

    if(++server->burst < NTP_BURST) {
        pollServer(server, 0);
        return;
    }

    // burst is complete, perform update calculations
    server->reach |= 1;
    server->stratum = headerNTP->stratum;
    server->lastResponse = CLK_MONOTONIC_INT();
    updateTime(servers);
}

static void pollServer(struct Server *server, int pingOnly) {
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
    copyMAC(headerEth->macDst, server->mac);

    // IPv4 Header
    IPv4_init(frame);
    headerIPv4->dst = server->addr;
    headerIPv4->src = ipAddress;
    headerIPv4->proto = IP_PROTO_UDP;

    // UDP Header
    headerUDP->portSrc = __builtin_bswap16(NTP_PORT);
    headerUDP->portDst = __builtin_bswap16(NTP_PORT);

    // set type to client request
    headerNTP->version = 4;
    headerNTP->mode = 3;
    // set reference ID
    memcpy(headerNTP->refID, "GPS", 4);
    // set TX time
    if(pingOnly == 0) {
        uint64_t *stamps = server->stamps + (server->burst << 2);
        stamps[0] = CLK_TAI();
        headerNTP->txTime[0] = __builtin_bswap32(((uint32_t *) stamps)[1]);
        headerNTP->txTime[1] = __builtin_bswap32(((uint32_t *) stamps)[0]);
    }

    // transmit request
    UDP_finalize(frame, NTP4_SIZE);
    IPv4_finalize(frame, NTP4_SIZE);
    NET_transmit(txDesc, NTP4_SIZE);
}
