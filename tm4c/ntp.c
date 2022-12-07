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
#define NTP_PRECISION (-24)
#define NTP_RING_MASK (15)
#define NTP_RING_SIZE (16)

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

static uint32_t ntpEra = 0;
static uint64_t ntpOffset = 0;
static float clockOffset;
static float clockDrift;

#define SERVER_COUNT (8)
struct Server {
    int head;
    float ringOffset[NTP_RING_SIZE];
    float ringDelay[NTP_RING_SIZE];
    float ringDrift[NTP_RING_SIZE];
    float meanOffset, varOffset;
    float meanDelay, varDelay;
    float meanDrift, varDrift;
    float weight, currentOffset;
    uint64_t stamps[NTP_BURST << 2];
    int64_t asym;
    uint64_t offset;
    uint64_t update;
    uint32_t nextPoll;
    uint32_t attempts;
    uint32_t addr;
    uint8_t mac[6];
    uint16_t reach;
    uint8_t stratum;
    uint8_t burst;
} servers[SERVER_COUNT];

static void processResponse(uint8_t *frame);
static void pollServer(struct Server *server, int pingOnly);
static void updateTracking(struct Server *server);

static void processRequest(uint8_t *frame, int flen);
static void processRequest0(const uint8_t *frame, struct HEADER_NTPv4 *headerNTP);
static void followupRequest0(uint8_t *frame, int flen);
static void processRequest4(const uint8_t *frame, struct HEADER_NTPv4 *headerNTP);

void NTP_init() {
    memset(servers, 0, sizeof(servers));
    UDP_register(NTP_PORT, processRequest);
    servers[0].addr = 0xC803A8C0;
}

uint64_t NTP_offset() {
    return ntpOffset;
}

float NTP_clockOffset() {
    return clockOffset;
}

float NTP_clockDrift() {
    return clockDrift;
}

void NTP_date(uint64_t clkMono, uint32_t *ntpDate) {
    uint64_t rollover = ((uint32_t *)&clkMono)[1];
    rollover += ((uint32_t *)&ntpOffset)[1];
    clkMono += ntpOffset;
    ntpDate[0] = ntpEra + ((uint32_t *)&rollover)[1];
    ntpDate[1] = __builtin_bswap32(((uint32_t *)&clkMono)[1]);
    ntpDate[2] = __builtin_bswap32(((uint32_t *)&clkMono)[0]);
    ntpDate[3] = 0;
}

void processRequest(uint8_t *frame, int flen) {
    // discard malformed packets
    if(flen < NTP4_SIZE) return;
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
        NET_setTxCallback(txDesc, followupRequest0);
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
    headerNTP->precision = NTP_PRECISION;
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

static void followupRequest0(uint8_t *frame, int flen) {
    // map headers
    struct FRAME_ETH *headerEth = (struct FRAME_ETH *) frame;
    struct HEADER_IPv4 *headerIPv4 = (struct HEADER_IPv4 *) (headerEth + 1);
    struct HEADER_UDP *headerUDP = (struct HEADER_UDP *) (headerIPv4 + 1);
    struct HEADER_NTPv4 *headerNTP = (struct HEADER_NTPv4 *) (headerUDP + 1);

    // guard against buffer overruns
    if(flen < NTP4_SIZE) return;
    if(headerEth->ethType != ETHTYPE_IPv4) return;
    if(headerIPv4->proto != IP_PROTO_UDP) return;
    if(headerUDP->portSrc != __builtin_bswap16(NTP_PORT)) return;
    if(headerNTP->version != 0) return;

    // append hardware transmit time
    uint32_t txTime[2];
    NET_getTxTimeRaw(frame, txTime);
    headerNTP->txTime[0] = txTime[0];
    headerNTP->txTime[1] = txTime[1];

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
    // transmit followup packet
    NET_transmit(txDesc, flen);
}

static void processRequest4(const uint8_t *frame, struct HEADER_NTPv4 *headerNTP) {
    // retrieve rx time
    uint64_t rxTime = NET_getRxTime(frame) + ntpOffset;

    // set type to server response
    headerNTP->mode = 4;
    // indicate that the time is not currently set
    headerNTP->status = GPSDO_isLocked() ? 0 : 3;
    // set other header fields
    headerNTP->stratum = GPSDO_isLocked() ? 1 : 16;
    headerNTP->precision = NTP_PRECISION;
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
    uint64_t refTime = CLK_TAI() + ntpOffset;
    headerNTP->refTime[0] = __builtin_bswap32(((uint32_t *) &refTime)[1]);
    headerNTP->refTime[1] = 0;
    // set RX time
    headerNTP->rxTime[0] = __builtin_bswap32(((uint32_t *) &rxTime)[1]);
    headerNTP->rxTime[1] = __builtin_bswap32(((uint32_t *) &rxTime)[0]);
    // set TX time
    uint64_t txTime = CLK_TAI() + ntpOffset;
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
    if(isNullMAC(server->mac)) {
        // determine if server is outside of current subnet
        if((server->addr ^ ipAddress) & ipSubnet)
            ARP_request(ipGateway, arpCallback);
        else
            ARP_request(server->addr, arpCallback);
        return;
    }

    // remove the server if it has high packet loss
    if(server->attempts > 16 && server->reach != 0xFFFF) {
        // count bits
        uint32_t cnt = server->reach;
        cnt -= (cnt >> 1) & 0x55555555;
        cnt = (cnt & 0x33333333) + ((cnt >> 2) & 0x33333333);
        cnt += cnt >> 4; cnt &= 0x0F0F0F0F;
        cnt += cnt >> 8; cnt &= 0xFF;
        // remove server if too many lost packets
        if(cnt < 9) {
            memset(server, 0, sizeof(struct Server));
            return;
        }
    }

    // update timing metrics 3 seconds after polling window
    if((now & 63) == 11)
        updateTracking(server);

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
    server->nextPoll = nextPoll;

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
        runServer(servers + i, now);

    // update tracking 4 seconds after polling window
    if((now & 63) != 12) return;

    // update server weights
    float norm = 0;
    for(int i = 0; i < SERVER_COUNT; i++) {
        if(servers[i].reach == 0) {
            servers[i].weight = 0;
            continue;
        }

        int reach = servers[i].reach;
        int elapsed = 0, quality = 0;
        for(int shift = 0; shift < 16; shift++) {
            if((reach >> shift) & 1) {
                elapsed += shift;
                ++quality;
            }
        }
        elapsed <<= 2;
        elapsed += 2;
        servers[i].currentOffset = servers[i].meanOffset;
        servers[i].currentOffset += servers[i].meanDrift * (float) elapsed;

        float weight = 0;
        if(servers[i].varDrift > 0) {
            weight = (float) elapsed;
            weight *= servers[i].varDrift;
            weight = sqrtf(weight);
            weight = 1.0f / weight;
            weight *= (float) quality;
        }
        servers[i].weight = weight;
        norm += weight;
    }
    clockDrift = 0;
    clockOffset = 0;
    if(norm > 0) {
        norm = 1.0f / norm;
        for(int i = 0; i < SERVER_COUNT; i++) {
            servers[i].weight *= norm;
            clockDrift += servers[i].weight * servers[i].meanDrift;
            clockOffset += servers[i].weight * servers[i].currentOffset;
        }
    }

    // compute mean offset
    uint64_t offset = servers[0].offset;
    int64_t diff = 0;
//    for(int i = 1; i < SERVER_COUNT; i++)
//        diff += (int64_t) (servers[i].offset - offset);
//    offset += diff >> 3;

    diff = (int64_t) (offset - ntpOffset);
    if(((int32_t *) &diff)[1] < 0 && ((uint32_t *) &diff)[0] <= (1 << 31))
        ((int32_t *) &diff)[1] -= 1;
    else if(((uint32_t *) &diff)[0] >= (1 << 31))
        ((int32_t *) &diff)[1] += 1;
    if(((int32_t *) &diff)[1] != 0)
        ((uint32_t *) &ntpOffset)[1] += ((int32_t *) &diff)[1];
}

char* NTP_servers(char *tail) {
    char tmp[32];

    tail = append(tail, "ntp servers: ");
    tail = append(tail, NTP_POOL_FQDN);
    tail = append(tail, "\n");

    // header
    tail = append(tail, "    ");
    tail = append(tail, "address        ");
    tail = append(tail, "  ");
    tail = append(tail, "stratum");
    tail = append(tail, "  ");
    tail = append(tail, "reach");
    tail = append(tail, "  ");
    tail = append(tail, "next");
    tail = append(tail, "  ");
    tail = append(tail, "offset (ms)");
    tail = append(tail, "  ");
    tail = append(tail, "asymm (ms)");
    tail = append(tail, "  ");
    tail = append(tail, "delay (ms)");
    tail = append(tail, "  ");
    tail = append(tail, "jitter (ms)");
    tail = append(tail, "  ");
    tail = append(tail, "drift (ppm)");
    tail = append(tail, "  ");
    tail = append(tail, "skew (ppm)");
    tail = append(tail, "  ");
    tail = append(tail, "weight");
    tail = append(tail, "\n");

    uint32_t now = CLK_MONOTONIC_INT();
    for(int i = 0; i < SERVER_COUNT; i++) {
        if(servers[i].addr == 0) continue;

        tail = append(tail, "  - ");
        char *pad = tail + 17;
        tail = addrToStr(servers[i].addr, tail);
        while(tail < pad)
            *(tail++) = ' ';

        tmp[toDec(servers[i].stratum, 7, ' ', tmp)] = 0;
        tail = append(tail, tmp);
        tail = append(tail, "  ");
        tmp[toOct(servers[i].reach & 077777, 5, '0', tmp)] = 0;
        tail = append(tail, tmp);
        tail = append(tail, "  ");
        tmp[toDec(servers[i].nextPoll - now, 4, ' ', tmp)] = 0;
        tail = append(tail, tmp);
        tail = append(tail, "  ");

        tmp[fmtFloat(1e3f * servers[i].currentOffset, 11, 3, tmp)] = 0;
        tail = append(tail, tmp);
        tail = append(tail, "  ");

        int32_t asym = (int32_t) servers[i].asym;
        tmp[fmtFloat((1e3f * 0x1p-32f) * (float) asym, 10, 3, tmp)] = 0;
        tail = append(tail, tmp);
        tail = append(tail, "  ");

        tmp[fmtFloat(servers[i].meanDelay * 1e3f, 10, 3, tmp)] = 0;
        tail = append(tail, tmp);
        tail = append(tail, "  ");

        tmp[fmtFloat(sqrtf(servers[i].varDelay) * 1e3f, 11, 3, tmp)] = 0;
        tail = append(tail, tmp);
        tail = append(tail, "  ");

        tmp[fmtFloat(servers[i].meanDrift * 1e6f, 11, 3, tmp)] = 0;
        tail = append(tail, tmp);
        tail = append(tail, "  ");

        tmp[fmtFloat(sqrtf(servers[i].varDrift) * 1e6f, 10, 3, tmp)] = 0;
        tail = append(tail, tmp);
        tail = append(tail, "  ");

        tmp[fmtFloat(servers[i].weight, 6, 4, tmp)] = 0;
        tail = append(tail, tmp);
        tail = append(tail, "\n");
    }
    return tail;
}

void computeMeanVar(const float *ring, float *mean, float *var) {
    float _mean = 0;
    for(int i = 0; i < NTP_RING_SIZE; i++)
        _mean += ring[i];
    _mean /= NTP_RING_SIZE;

    float _var = 0;
    for(int i = 0; i < NTP_RING_SIZE; i++) {
        float diff = ring[i] - _mean;
        _var += diff * diff;
    }
    _var /= NTP_RING_SIZE;
    _var *= 4;

    int cnt = 0;
    *mean = 0, *var = 0;
    for(int i = 0; i < NTP_RING_SIZE; i++) {
        float diff = ring[i] - _mean;
        diff *= diff;
        if(diff < _var) {
            *mean += ring[i];
            *var += diff;
            ++cnt;
        }
    }
    *mean /= (float) cnt;
    *var /= (float) cnt;
}

static void updateTracking(struct Server *server) {
    // burst must be complete
    if(server->burst < NTP_BURST) return;

    // advance ring
    ++server->head;
    server->head &= NTP_RING_MASK;
    const int head = server->head;

    // compute delay and offset
    int64_t _adjust = 0;
    uint64_t _delay = 0;
    uint64_t offset = server->stamps[1] - server->stamps[0];
    for(int i = 0; i < NTP_BURST; i++) {
        const uint64_t *set = server->stamps + (i << 2);
        // accumulate delay
        _delay += set[3] - set[2];
        _delay += set[1] - set[0];
        // accumulate offset adjustment
        _adjust += (int64_t) ((set[1] - set[0]) - offset);
        _adjust += (int64_t) ((set[2] - set[3]) - offset);
    }
    // normalize delay
    _delay >>= NTP_BURST_BITS + 1;
    // adjust offset
    offset += _adjust >> (NTP_BURST_BITS + 1);

    // compute asymmetry
    uint64_t asym = 0;
    for(int i = 0; i < NTP_BURST; i++) {
        const uint64_t *set = server->stamps + (i << 2);
        asym += set[3] - set[1];
        asym += set[2] - set[0];
    }
    // normalize delay
    asym >>= NTP_BURST_BITS + 1;
    server->asym = (int64_t) (asym - _delay);
//    offset += asym;

    // compute update time
    uint64_t update = server->stamps[(NTP_BURST << 2) - 1];
    update += ((int64_t) (server->stamps[0] - update)) >> 1;

    // update ring
    server->ringOffset[head] = 0x1p-32f * (float) (int32_t) (offset - ntpOffset);
    server->ringDelay[head] = 0x1p-32f * (float) (uint32_t) _delay;
    if(server->reach == 1) {
        for(int i = 0; i < NTP_RING_SIZE; i++) {
            int j = (i + head) & NTP_RING_MASK;
            server->ringOffset[j] = server->ringOffset[head];
            server->ringDelay[j] = server->ringDelay[head];
        }
    }

    // compute drift
    if(server->update != 0) {
        uint64_t interval = update - server->update;
        uint32_t ipart = ((uint32_t *) &interval)[1];
        uint8_t shift = 0;
        while(ipart >> shift) ++shift;
        float drift = (float) (int32_t) (offset - server->offset);
        drift /= (float) (uint32_t) (interval >> shift);
        drift /= (float) (1 << shift);
        server->ringDrift[head] = drift;
        computeMeanVar(server->ringDrift, &(server->meanDrift), &(server->varDrift));
    }

    // update server status
    server->offset = offset;
    server->update = update;

    // update stats
    computeMeanVar(server->ringOffset, &(server->meanOffset), &(server->varOffset));
    computeMeanVar(server->ringDelay, &(server->meanDelay), &(server->varDelay));
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
            server = servers + i;
            break;
        }
    }
    // ignore unsolicited packets
    if(server == NULL) return;
    if(server->burst >= NTP_BURST) return;
    // map timestamps
    uint64_t *stamps = server->stamps + (server->burst << 2);

    // verify client TX timestamp
    uint64_t txTime;
    ((uint32_t *)&txTime)[1] = __builtin_bswap32(headerNTP->origTime[0]);
    ((uint32_t *)&txTime)[0] = __builtin_bswap32(headerNTP->origTime[1]);
    // discard ping responses
    if(txTime == 0) return;
    // discard stale or duplicate packets
    if(txTime != stamps[0]) return;
    // move hardware TX timestamp to correct location
    stamps[0] = stamps[1];

    // copy server TX timestamp
    ((uint32_t *)(stamps + 1))[1] = __builtin_bswap32(headerNTP->rxTime[0]);
    ((uint32_t *)(stamps + 1))[0] = __builtin_bswap32(headerNTP->rxTime[1]);
    // copy server TX timestamp
    ((uint32_t *)(stamps + 2))[1] = __builtin_bswap32(headerNTP->txTime[0]);
    ((uint32_t *)(stamps + 2))[0] = __builtin_bswap32(headerNTP->txTime[1]);
    // record client RX timestamp
    stamps[3] = NET_getRxTime(frame);

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
}

static void pollTxCallback(uint8_t *frame, int flen) {
    // map headers
    struct FRAME_ETH *headerEth = (struct FRAME_ETH *) frame;
    struct HEADER_IPv4 *headerIPv4 = (struct HEADER_IPv4 *) (headerEth + 1);
    struct HEADER_UDP *headerUDP = (struct HEADER_UDP *) (headerIPv4 + 1);
    struct HEADER_NTPv4 *headerNTP = (struct HEADER_NTPv4 *) (headerUDP + 1);

    // guard against buffer overruns
    if(flen < NTP4_SIZE) return;
    if(headerEth->ethType != ETHTYPE_IPv4) return;
    if(headerIPv4->proto != IP_PROTO_UDP) return;
    if(headerUDP->portSrc != __builtin_bswap16(NTP_PORT)) return;
    if(headerUDP->portDst != __builtin_bswap16(NTP_PORT)) return;
    if(headerNTP->version != 4) return;

    // validate remote address
    struct Server *server = NULL;
    for(int i = 0; i < SERVER_COUNT; i++) {
        if(servers[i].addr == headerIPv4->dst) {
            server = servers + i;
            break;
        }
    }
    if(server == NULL) return;

    // locate timestamp record
    uint64_t txTime;
    ((uint32_t *)&txTime)[1] = __builtin_bswap32(headerNTP->origTime[0]);
    ((uint32_t *)&txTime)[0] = __builtin_bswap32(headerNTP->origTime[1]);
    for(int i = 0; i < NTP_BURST; i++) {
        uint64_t *set = server->stamps + (i << 2);
        if(set[0] == txTime) {
            // update timestamp if unmodified
            if(set[1] == set[0])
                set[1] = NET_getTxTime(frame);
            break;
        }
    }
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
    headerNTP->poll = 4;
    headerNTP->precision = NTP_PRECISION;
    // set reference ID
    memcpy(headerNTP->refID, "GPS", 4);
    // set reference timestamp
    uint64_t refTime = CLK_TAI() + ntpOffset;
    headerNTP->refTime[0] = __builtin_bswap32(((uint32_t *) &refTime)[1]);
    headerNTP->refTime[1] = 0;
    // set TX time
    if(pingOnly == 0) {
        uint64_t *stamps = server->stamps + (server->burst << 2);
        stamps[0] = CLK_TAI();
        stamps[1] = stamps[0];
        headerNTP->txTime[0] = __builtin_bswap32(((uint32_t *) stamps)[1]);
        headerNTP->txTime[1] = __builtin_bswap32(((uint32_t *) stamps)[0]);
        NET_setTxCallback(txDesc, pollTxCallback);
    }

    // transmit request
    UDP_finalize(frame, NTP4_SIZE);
    IPv4_finalize(frame, NTP4_SIZE);
    NET_transmit(txDesc, NTP4_SIZE);
}
