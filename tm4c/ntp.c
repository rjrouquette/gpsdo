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
#define NTP_MAX_STRATUM (4)
#define NTP_REF_GPS (0x00535047) // "GPS"
#define NTP_LI_ALARM (3)
#define NTP_POLL_BITS (6)
#define NTP_POLL_MASK (63)
#define NTP_POLL_INTV (64)
#define NTP_POLL_RAND ((STCURRENT.CURRENT >> 8) & 7) // employs scheduling uncertainty
#define NTP_UTC_OFFSET (2208988800)
#define NTP_STAT_RATE (0x1p-3f)

struct PACKED HEADER_NTPv4 {
    uint16_t mode: 3;
    uint16_t version: 3;
    uint16_t status: 2;
    uint8_t stratum;
    uint8_t poll;
    int8_t precision;
    uint32_t rootDelay;
    uint32_t rootDispersion;
    uint32_t refID;
    uint64_t refTime;
    uint64_t origTime;
    uint64_t rxTime;
    uint64_t txTime;
};
_Static_assert(sizeof(struct HEADER_NTPv4) == 48, "HEADER_NTPv4 must be 48 bytes");

static uint32_t ntpEra = 0;
static uint64_t ntpOffset = 0;
static float clockOffset;
static float clockDrift;
static float clockSkew;
static int clockStratum = 16;
static int32_t rootDelay;
static int32_t rootDispersion;
static uint32_t refId;
static int leapIndicator;

#define SERVER_COUNT (8)
struct Server {
    float meanDelay, varDelay;
    float meanDrift, varDrift;
    float weight, currentOffset;
    uint64_t stamps[NTP_BURST << 2];
    uint64_t offset;
    uint64_t update;
    int32_t rootDelay;
    int32_t rootDispersion;
    int pollSlot;
    uint32_t attempts;
    uint32_t addr;
    uint16_t reach;
    uint8_t mac[6];
    uint8_t leapIndicator;
    uint8_t stratum;
    uint8_t burst;
} servers[SERVER_COUNT];

static void processResponse(uint8_t *frame);
static void pollServer(struct Server *server, int pingOnly);
static void resetServer(struct Server *server);
static void updateMeanVar(float rate, float value, float *mean, float *var);

static void processRequest(uint8_t *frame, int flen);
static void respond0(struct HEADER_NTPv4 *headerNTP, uint64_t rxTime);
static void followup0(uint8_t *frame, int flen);
static void respond4(struct HEADER_NTPv4 *headerNTP, uint64_t rxTime);

static void callbackARP(uint32_t remoteAddress, uint8_t *macAddress);
static void callbackDNS(uint32_t addr);


// NTP tasks
static void runPoll();
static void runShuffle();
static void runStats();
static void runTracking();
static void runAggregate();
static void runPrune();
static void runDNS();
static void runARP();
static void runPing();

// time-slot task assignments
typedef void (*SlotTask) (void);
struct TaskSlot {
    SlotTask task;
    int end;
} taskSlot[NTP_POLL_INTV];

// NTP process state
static struct NtpProcess {
    uint32_t nowMono;
    int timeSlot;
    int iter;
    int end;
} ntpProcess;



void NTP_init() {
    memset(servers, 0, sizeof(servers));
    UDP_register(NTP_PORT, processRequest);

    // configure state machine schedule
    memset(taskSlot, 0, sizeof(taskSlot));
    // polling tasks
    for(int i = 0; i < 8; i++) {
        taskSlot[i].task = runPoll;
        taskSlot[i].end = SERVER_COUNT;
    }
    // shuffle poll time slots
    taskSlot[8].task = runShuffle;
    taskSlot[8].end = SERVER_COUNT;
    // update tracking stats
    taskSlot[10].task = runStats;
    taskSlot[10].end = SERVER_COUNT;
    // update tracking stats
    taskSlot[11].task = runTracking;
    taskSlot[11].end = SERVER_COUNT;
    // update tracking stats
    taskSlot[12].task = runAggregate;
    taskSlot[12].end = 1;
    // prune erratic servers
    taskSlot[13].task = runPrune;
    taskSlot[13].end = SERVER_COUNT;
    // expand server list
    taskSlot[14].task = runDNS;
    taskSlot[14].end = 1;
    // MAC address resolution
    taskSlot[20].task = runARP;
    taskSlot[20].end = SERVER_COUNT;
    // pre-poll server ping
    taskSlot[63].task = runPing;
    taskSlot[63].end = SERVER_COUNT;
}

uint64_t NTP_offset() {
    return ntpOffset;
}

void NTP_setEpochOffset(uint32_t offset) {
    ((uint32_t *) &ntpOffset)[1] = offset + NTP_UTC_OFFSET;
}

float NTP_clockOffset() {
    return clockOffset;
}

float NTP_clockDrift() {
    return clockDrift;
}

float NTP_clockSkew() {
    return clockSkew;
}

int NTP_clockStratum() {
    return clockStratum;
}

int NTP_leapIndicator() {
    return leapIndicator;
}

float NTP_rootDelay() {
    return 0x1p-16f * (float) rootDelay;
}

float NTP_rootDispersion() {
    return 0x1p-16f * (float) rootDispersion;
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
    tail = append(tail, "leap");
    tail = append(tail, "  ");
    tail = append(tail, "reach");
    tail = append(tail, "  ");
    tail = append(tail, "next");
    tail = append(tail, "  ");
    tail = append(tail, "offset (ms)");
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

        tmp[toDec(servers[i].leapIndicator, 4, ' ', tmp)] = 0;
        tail = append(tail, tmp);
        tail = append(tail, "  ");

        tmp[toOct(servers[i].reach & 077777, 5, '0', tmp)] = 0;
        tail = append(tail, tmp);
        tail = append(tail, "  ");

        tmp[toDec((servers[i].pollSlot - now) & NTP_POLL_MASK, 4, ' ', tmp)] = 0;
        tail = append(tail, tmp);
        tail = append(tail, "  ");

        tmp[fmtFloat(1e3f * servers[i].currentOffset, 11, 3, tmp)] = 0;
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

// main process entry point
void NTP_run() {
    // set current system time
    ntpProcess.nowMono = CLK_MONOTONIC_INT();
    int timeSlot = (int) ntpProcess.nowMono & NTP_POLL_MASK;
    // advance process state
    struct TaskSlot *slot = taskSlot + timeSlot;
    if(timeSlot != ntpProcess.timeSlot) {
        ntpProcess.iter = 0;
        ntpProcess.end = slot->end;
        ntpProcess.timeSlot = timeSlot;
    }
    // dispatch appropriate task
    if(ntpProcess.iter < ntpProcess.end) {
        (*slot->task)();
        ++ntpProcess.iter;
    }
}

static void runPoll() {
    // verify server poll slot
    struct Server *server = servers + ntpProcess.iter;
    if(server->pollSlot != ntpProcess.timeSlot)
        return;
    // verify that server is configured
    if(server->addr == 0)
        return;
    // advance reach window
    server->reach <<= 1;
    ++(server->attempts);
    // send poll request
    server->burst = 0;
    memset(server->stamps, 0, sizeof(server->stamps));
    pollServer(server, 0);
}

static void runShuffle() {
    // shuffle server poll slot
    struct Server *server = servers + ntpProcess.iter;
    server->pollSlot = NTP_POLL_RAND;
}

static void runStats() {
    struct Server *server = servers + ntpProcess.iter;
    if(server->addr == 0) return;

    // burst must be complete
    if(server->burst < NTP_BURST) {
        // discard late arrivals
        server->burst = NTP_BURST;
        return;
    }

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

    // compute update time
    uint64_t update = server->stamps[(NTP_BURST << 2) - 1];
    update += ((int64_t) (server->stamps[0] - update)) >> 1;

    // update offset and delay
    server->currentOffset = 0x1p-32f * (float) *(int32_t *) &offset;
    const float delay = 0x1p-32f * (float) *(uint32_t *) &_delay;
    updateMeanVar(NTP_STAT_RATE, delay, &(server->meanDelay), &(server->varDelay));

    // compute drift
    if(server->update != 0) {
        uint64_t interval = update - server->update;
        uint32_t ipart = ((uint32_t *) &interval)[1];
        uint8_t shift = 0;
        while(ipart >> shift) ++shift;
        float drift = (float) (int32_t) (offset - server->offset);
        drift /= (float) (uint32_t) (interval >> shift);
        drift /= (float) (1 << shift);
        updateMeanVar(NTP_STAT_RATE, drift, &(server->meanDrift), &(server->varDrift));
    }

    // update server status
    server->offset = offset;
    server->update = update;
}

static void runTracking() {
    struct Server *server = servers + ntpProcess.iter;
    if(server->addr == 0) return;

    // check for failure states
    if(
            server->leapIndicator == NTP_LI_ALARM ||
            server->stratum == 0 ||
            server->stratum > NTP_MAX_STRATUM
    ) {
        // unset reach bit if server stratum is too poor
        server->reach &= 0xFFFE;
        server->weight = 0;
        return;
    }

    float weight = 0;
    if((server->reach & 1) && server->varDelay > 0) {
        weight = 1.0f / sqrtf(server->varDelay * (float) server->stratum);
    }
    if(!isfinite(weight))
        weight = 0;
    server->weight = weight;
}

static void runAggregate() {
    // normalize server weights
    float norm = 0;
    for(int i = 0; i < SERVER_COUNT; i++)
        norm += servers[i].weight;
    if(norm > 0) {
        float mean = 0;
        for(int i = 0; i < SERVER_COUNT; i++)
            mean += servers[i].currentOffset * servers[i].weight;
        mean /= norm;

        float var = 0;
        for(int i = 0; i < SERVER_COUNT; i++) {
            float diff = servers[i].currentOffset - mean;
            diff *= diff;
            var += diff * servers[i].weight;
        }
        var /= norm;
        var *= 4;

        norm = 0;
        for(int i = 0; i < SERVER_COUNT; i++) {
            // drop server if offset is too high
            float diff = servers[i].currentOffset - mean;
            diff *= diff;
            if(diff < var)
                norm += servers[i].weight;
            else
                servers[i].weight = 0;
        }
    }

    refId = 0;
    clockDrift = 0;
    clockOffset = 0;
    clockSkew = 0;
    float _stratum = 0;
    float _delay = 0;
    float _dispersion = 0;
    float best = 0;
    int li[4] = {0,0,0,0};
    if(norm > 0) {
        norm = 1.0f / norm;
        for(int i = 0; i < SERVER_COUNT; i++) {
            servers[i].weight *= norm;
            const float weight = servers[i].weight;
            clockSkew += weight * servers[i].varDrift;
            clockDrift += weight * servers[i].meanDrift;
            clockOffset += weight * servers[i].currentOffset;
            _stratum += weight * (float) servers[i].stratum;
            _delay += weight * (servers[i].meanDelay + (0x1p-16f * (float) servers[i].rootDelay));
            const float z = 0x1p-16f * (float) servers[i].rootDispersion;
            _dispersion += weight * (servers[i].varDelay + (z * z));
            // set reference ID to server with best weight
            if(weight > best) {
                best = weight;
                refId = servers[i].addr;
            }
            // tally leap indicator votes
            if(weight > 0.05f)
                ++li[servers[i].leapIndicator];
        }
    }
    clockSkew = sqrtf(clockSkew);

    // update clock stratum
    if(GPSDO_isLocked()) {
        clockStratum = 1;
        rootDelay = 0;
        rootDispersion = 0;
    }
    else if(_stratum == 0) {
        clockStratum = 16;
        rootDelay = 0;
        rootDispersion = 0;
    }
    else {
        clockStratum = 1 + (int) roundf(_stratum);
        rootDelay = (int32_t) roundf(0x1p16f * _delay);
        rootDispersion = (int32_t) roundf(0x1p16f * sqrtf(_dispersion));
    }

    // set reference ID to GPS if stratum is 1
    if(clockStratum == 1)
        refId = NTP_REF_GPS;

    // tally leap indicator votes
    leapIndicator = 0;
    int bestLI = 0;
    for(int i = 0; i < 3; i++) {
        if(li[i] > bestLI) {
            bestLI = li[i];
            leapIndicator = i;
        }
    }

    // compute mean offset
    int64_t offset = 0;
    int32_t divisor = 0;
    for(int i = 0; i < SERVER_COUNT; i++) {
        int32_t weight = (int32_t) (0x1p24f * servers[i].weight);
        divisor += weight;
        int64_t _offset = (int64_t) servers[i].offset;
        _offset >>= 24;
        _offset *= weight;
        offset += _offset;
    }
    // correct for integer truncation
    int64_t twiddle = offset;
    twiddle >>= 24;
    twiddle *= (1<<24) - divisor;
    offset += twiddle;

    if(!GPSDO_isLocked()) {
        if (offset != 0) {
            // compute offset adjustment
            int64_t diff = offset - (int64_t) ntpOffset;
            // round to whole seconds
            if (((int32_t *) &diff)[1] < 0 && ((uint32_t *) &diff)[0] <= (1 << 31))
                ((int32_t *) &diff)[1] -= 1;
            else if (((uint32_t *) &diff)[0] >= (1 << 31))
                ((int32_t *) &diff)[1] += 1;
            // apply offset adjustment
            ((uint32_t *) &ntpOffset)[1] += ((int32_t *) &diff)[1];
        }
    }

    // notify GPSDO of ntp status for fail-over purposes
    if(norm > 0) {
        if (GPSDO_ntpUpdate(clockOffset, clockSkew)) {
            // reset server stats if clock was hard stepped
            for (int i = 0; i < SERVER_COUNT; i++)
                resetServer(servers + i);
        }
    }
}

static void runPrune() {
    // examine sever connection quality
    struct Server *server = servers + ntpProcess.iter;
    // verify that server is configured
    if(server->addr == 0)
        return;

    // check if server has alarm
    if(server->leapIndicator == NTP_LI_ALARM) {
        memset(server, 0, sizeof(struct Server));
        return;
    }

    // check if server has any missed polls
    if(server->attempts < 16) return;
    if(server->reach == 0xFFFF)  return;
    // tally successful poll attempts
    uint32_t cnt = server->reach;
    cnt -= (cnt >> 1) & 0x55555555;
    cnt = (cnt & 0x33333333) + ((cnt >> 2) & 0x33333333);
    cnt += cnt >> 4; cnt &= 0x0F0F0F0F;
    cnt += cnt >> 8; cnt &= 0xFF;
    // remove server if too many lost packets
    if(cnt < 11)
        memset(server, 0, sizeof(struct Server));
}

static void runDNS() {
    for(int i = 0; i < SERVER_COUNT; i++) {
        if (servers[i].addr == 0) {
            DNS_lookup(NTP_POOL_FQDN, callbackDNS);
            break;
        }
    }
}

static void runARP() {
    // update server tracking stats
    struct Server *server = servers + ntpProcess.iter;
    // verify that server is configured
    if(server->addr == 0)
        return;
    // perform ARP request if MAC address is missing
    if(!isNullMAC(server->mac)) return;
    // determine if server is outside of current subnet
    if((server->addr ^ ipAddress) & ipSubnet)
        ARP_request(ipGateway, callbackARP);
    else
        ARP_request(server->addr, callbackARP);
}

static void runPing() {
    // verify server poll slot
    struct Server *server = servers + ntpProcess.iter;
    if(server->pollSlot != ntpProcess.timeSlot)
        return;
    // verify that server is configured
    if(server->addr == 0)
        return;
    // ping server in advance of poll request
    pollServer(server, 1);
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
    // indicate time-server activity
    LED_act0();

    // retrieve rx time
    uint64_t rxTime = NET_getRxTime(frame);
    // get TX descriptor
    int txDesc = NET_getTxDesc();
    if(txDesc < 0) return;
    // duplicate packet for sending
    uint8_t *temp = NET_getTxBuff(txDesc);
    memcpy(temp, frame, flen);
    frame = temp;
    // return the response directly to the sender
    UDP_returnToSender(frame, ipAddress, NTP_PORT);
    // remap headers
    headerEth = (struct FRAME_ETH *) frame;
    headerIPv4 = (struct HEADER_IPv4 *) (headerEth + 1);
    headerUDP = (struct HEADER_UDP *) (headerIPv4 + 1);
    headerNTP = (struct HEADER_NTPv4 *) (headerUDP + 1);

    // process message
    if(headerNTP->version == 0) {
        respond0(headerNTP, rxTime);
        NET_setTxCallback(txDesc, followup0);
    }
    else {
        respond4(headerNTP, rxTime);
    }

    // finalize packet
    UDP_finalize(frame, flen);
    IPv4_finalize(frame, flen);
    // transmit packet
    NET_transmit(txDesc, flen);
}

static void respond0(struct HEADER_NTPv4 *headerNTP, uint64_t rxTime) {
    // set type to server response
    headerNTP->mode = 4;
    headerNTP->status = leapIndicator;
    // set stratum and precision
    headerNTP->stratum = clockStratum;
    headerNTP->precision = NTP_PRECISION;
    // set reference ID
    headerNTP->refID = refId;
    // set root delay
    headerNTP->rootDelay = rootDelay;
    // set root dispersion
    headerNTP->rootDispersion = rootDispersion;
    // set origin timestamp
    headerNTP->origTime = headerNTP->txTime;
    // set RX time
    headerNTP->rxTime = rxTime;
    // set reference time
    headerNTP->refTime = GPSDO_timeTrimmed();
    // no TX time
    headerNTP->txTime = 0;
}

static void followup0(uint8_t *frame, int flen) {
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
    headerNTP->txTime = NET_getTxTime(frame);

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

static void respond4(struct HEADER_NTPv4 *headerNTP, uint64_t rxTime) {
    // set type to server response
    headerNTP->mode = 4;
    headerNTP->status = leapIndicator;
    // set stratum and precision
    headerNTP->stratum = clockStratum;
    headerNTP->precision = NTP_PRECISION;
    // set reference ID
    headerNTP->refID = refId;
    // set root delay
    headerNTP->rootDelay = __builtin_bswap32(rootDelay);
    // set root dispersion
    headerNTP->rootDispersion = __builtin_bswap32(rootDispersion);
    // set origin timestamp
    headerNTP->origTime = headerNTP->txTime;
    // set reference timestamp
    headerNTP->refTime = __builtin_bswap64(GPSDO_timeTrimmed() + ntpOffset);
    // set RX time
    headerNTP->rxTime = __builtin_bswap64(rxTime + ntpOffset);
    // set TX time
    headerNTP->txTime = __builtin_bswap64(CLK_TAI() + ntpOffset);
}

static void callbackARP(uint32_t remoteAddress, uint8_t *macAddress) {
    // special case for external addresses
    if(remoteAddress == ipGateway) {
        for(int i = 0; i < SERVER_COUNT; i++) {
            if((servers[i].addr ^ ipAddress) & ipSubnet) {
                copyMAC(servers[i].mac, macAddress);
            }
        }
        return;
    }
    // find matching server
    for(int i = 0; i < SERVER_COUNT; i++) {
        if(servers[i].addr == remoteAddress) {
            copyMAC(servers[i].mac, macAddress);
            break;
        }
    }
}

static void callbackDNS(uint32_t addr) {
    // prevent duplicate entries
    for (int i = 0; i < SERVER_COUNT; i++) {
        if(servers[i].addr == addr)
            return;
    }
    // fill empty server slot
    for (int i = 0; i < SERVER_COUNT; i++) {
        if (servers[i].addr == 0) {
            servers[i].addr = addr;
            break;
        }
    }
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
    uint64_t txTime = __builtin_bswap64(headerNTP->origTime);
    // discard ping responses
    if(txTime == 0) return;
    // discard stale or duplicate packets
    if(txTime != stamps[0]) return;
    // move hardware TX timestamp to correct location
    stamps[0] = stamps[1];

    // copy server TX timestamp
    stamps[1] = __builtin_bswap64(headerNTP->rxTime);
    // copy server TX timestamp
    stamps[2] = __builtin_bswap64(headerNTP->txTime);
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
    server->rootDelay = (int32_t) __builtin_bswap32(headerNTP->rootDelay);
    server->rootDispersion = (int32_t) __builtin_bswap32(headerNTP->rootDispersion);
    server->leapIndicator = headerNTP->status;
}

static void pollTxCallback(uint8_t *frame, int flen) {
    // map headers
    struct FRAME_ETH *headerEth = (struct FRAME_ETH *) frame;
    struct HEADER_IPv4 *headerIPv4 = (struct HEADER_IPv4 *) (headerEth + 1);
    struct HEADER_UDP *headerUDP = (struct HEADER_UDP *) (headerIPv4 + 1);
    struct HEADER_NTPv4 *headerNTP = (struct HEADER_NTPv4 *) (headerUDP + 1);

    // guard against buffer overruns
    if(flen != NTP4_SIZE) return;

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
    uint64_t txTime = __builtin_bswap32(headerNTP->origTime);
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
    headerNTP->poll = NTP_POLL_BITS;
    // set stratum and precision
    headerNTP->stratum = clockStratum;
    headerNTP->precision = NTP_PRECISION;
    // set reference ID
    headerNTP->refID = refId;
    // set reference timestamp
    uint64_t refTime = CLK_TAI() + ntpOffset;
    headerNTP->refTime = __builtin_bswap64(refTime);
    // set TX time
    if(pingOnly == 0) {
        uint64_t *stamps = server->stamps + (server->burst << 2);
        stamps[0] = CLK_TAI();
        stamps[1] = stamps[0];
        headerNTP->txTime = __builtin_bswap64(stamps[0]);
        NET_setTxCallback(txDesc, pollTxCallback);
    }

    // transmit request
    UDP_finalize(frame, NTP4_SIZE);
    IPv4_finalize(frame, NTP4_SIZE);
    NET_transmit(txDesc, NTP4_SIZE);
}

static void updateMeanVar(float rate, float value, float *mean, float *var) {
    if(mean[0] == 0 && var[0] == 0) {
        mean[0] = value;
        var[0] = value * value;
    } else {
        float diff = value - mean[0];
        diff *= diff;
        if(diff <= var[0] * 4) {
            mean[0] += (value - mean[0]) * rate;
        }
        var[0] += (diff - var[0]) * rate;
    }
}

static void resetServer(struct Server *server) {
    // clear poll tracking
    server->attempts = 0;
    server->offset = 0;
    server->update = 0;
    server->reach = 0;
}
