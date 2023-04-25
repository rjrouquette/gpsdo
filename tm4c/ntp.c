//
// Created by robert on 5/21/22.
//

#include <memory.h>
#include <math.h>

#include "gpsdo.h"
#include "lib/chrony/candm.h"
#include "lib/clk.h"
#include "lib/format.h"
#include "lib/net.h"
#include "lib/net/arp.h"
#include "lib/net/dns.h"
#include "lib/net/dhcp.h"
#include "lib/net/eth.h"
#include "lib/net/ip.h"
#include "lib/net/udp.h"
#include "lib/net/util.h"
#include "lib/led.h"
#include "lib/rand.h"
#include "ntp.h"

#define NTP4_SIZE (UDP_DATA_OFFSET + 48)
#define NTP_SRV_PORT (123)
#define NTP_CLI_PORT (12345)
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
#define NTP_POLL_SLOTS (8)
#define NTP_POLL_PING (NTP_POLL_INTV - NTP_POLL_SLOTS)
#define NTP_POLL_RAND (RAND_next() & (NTP_POLL_SLOTS - 1))
#define NTP_UTC_OFFSET (2208988800)
#define NTP_STAT_RATE (0x1p-3f)
#define NTP_ACTIVE_THRESH (0.0001f)
#define NTP_ACTIVE_TIMEOUT (2048)

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
static struct Server {
    float meanDelay, varDelay;
    float meanDrift, varDrift;
    float weight, currentOffset;
    uint64_t prevTxSrv, prevRx;
    uint64_t stamps[NTP_BURST << 2];
    uint64_t offset;
    uint64_t update;
    int32_t rootDelay;
    int32_t rootDispersion;
    int pollSlot;
    uint32_t lastActive;
    uint32_t attempts;
    uint32_t addr;
    uint16_t reach;
    uint8_t mac[6];
    uint8_t leapIndicator;
    uint8_t stratum;
    uint8_t burst;
} servers[SERVER_COUNT];

// hash table for interleaved timestamps
#define XLEAVE_COUNT (1024)
static struct TsXleave {
    uint64_t rxTime;
    uint64_t txTime;
} tsXleave[XLEAVE_COUNT];
static inline int hashXleave(uint32_t addr) { return (int) (((addr * 0xDE9DB139) >> 22) & (XLEAVE_COUNT - 1)); }

static inline uint64_t ntpClock() { return CLK_TAI() + ntpOffset; }
static inline uint64_t ntpModified() { return GPSDO_updated() + ntpOffset; }
static inline uint64_t ntpTimeRx(uint8_t *frame) { return NET_getRxTime(frame) + ntpOffset; }
static inline uint64_t ntpTimeTx(uint8_t *frame) { return NET_getTxTime(frame) + ntpOffset; }

static void pollServer(struct Server *server, int pingOnly);
static void resetServer(struct Server *server);
static void updateMeanVar(float rate, float value, float *mean, float *var);

static void processRequest(uint8_t *frame, int flen);
static void processResponse(uint8_t *frame, int flen);

static void callbackARP(uint32_t remoteAddress, uint8_t *macAddress);
static void callbackDNS(uint32_t addr);

// process chrony status requests
static void processChrony(uint8_t *frame, int flen);

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
    UDP_register(NTP_SRV_PORT, processRequest);
    UDP_register(NTP_CLI_PORT, processResponse);
    // listen for crony status requests
    UDP_register(DEFAULT_CANDM_PORT, processChrony);

    // configure state machine schedule
    memset(taskSlot, 0, sizeof(taskSlot));
    // polling tasks
    for(int i = 0; i < 8; i++) {
        taskSlot[i].task = runPoll;
        taskSlot[i].end = SERVER_COUNT;
        // pre-poll server ping
        taskSlot[NTP_POLL_PING+i].task = runPing;
        taskSlot[NTP_POLL_PING+i].end = SERVER_COUNT;
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
}

uint64_t NTP_offset() {
    return ntpOffset;
}

void NTP_setOffsetTai(int offsetFromTai) {
    ((uint32_t *) &ntpOffset)[1] = offsetFromTai + NTP_UTC_OFFSET;
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

void NTP_date(uint32_t *ntpDate) {
    union fixed_32_32 now;
    now.full = ntpClock();
    ntpDate[0] = ntpEra;
    ntpDate[1] = __builtin_bswap32(now.ipart);
    ntpDate[2] = __builtin_bswap32(now.fpart);
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
    tail = append(tail, "st");
    tail = append(tail, "  ");
    tail = append(tail, "li");
    tail = append(tail, "  ");
    tail = append(tail, "reach");
    tail = append(tail, "  ");
    tail = append(tail, "next");
    tail = append(tail, "  ");
    tail = append(tail, "offset  ");
    tail = append(tail, "  ");
    tail = append(tail, "delay   ");
    tail = append(tail, "  ");
    tail = append(tail, "jitter  ");
    tail = append(tail, "  ");
    tail = append(tail, "drift   ");
    tail = append(tail, "  ");
    tail = append(tail, "skew    ");
    tail = append(tail, "  ");
    tail = append(tail, "weight");
    tail = append(tail, "\n");

    uint32_t now = CLK_MONOTONIC_INT();
    for(int i = 0; i < SERVER_COUNT; i++) {
        if(servers[i].addr == 0) continue;

        tail = append(tail, (servers[i].weight == 0) ? "  - " : "  + ");
        char *pad = tail + 17;
        tail = addrToStr(servers[i].addr, tail);
        while(tail < pad)
            *(tail++) = ' ';

        tmp[toDec(servers[i].stratum, 2, ' ', tmp)] = 0;
        tail = append(tail, tmp);
        tail = append(tail, "  ");

        tmp[toDec(servers[i].leapIndicator, 2, ' ', tmp)] = 0;
        tail = append(tail, tmp);
        tail = append(tail, "  ");

        tmp[toOct(servers[i].reach & 077777, 5, '0', tmp)] = 0;
        tail = append(tail, tmp);
        tail = append(tail, "  ");

        tmp[toDec((servers[i].pollSlot - now) & NTP_POLL_MASK, 4, ' ', tmp)] = 0;
        tail = append(tail, tmp);
        tail = append(tail, "  ");

        tmp[fmtFloat(1e3f * servers[i].currentOffset, 8, 3, tmp)] = 0;
        tail = append(tail, tmp);
        tail = append(tail, "  ");

        tmp[fmtFloat(servers[i].meanDelay * 1e3f, 8, 3, tmp)] = 0;
        tail = append(tail, tmp);
        tail = append(tail, "  ");

        tmp[fmtFloat(sqrtf(servers[i].varDelay) * 1e3f, 8, 3, tmp)] = 0;
        tail = append(tail, tmp);
        tail = append(tail, "  ");

        tmp[fmtFloat(servers[i].meanDrift * 1e6f, 8, 3, tmp)] = 0;
        tail = append(tail, tmp);
        tail = append(tail, "  ");

        tmp[fmtFloat(sqrtf(servers[i].varDrift) * 1e6f, 8, 3, tmp)] = 0;
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

    // fast return to stratum 1 with PPS presence
    if(clockStratum != 1 && GPSDO_ppsPresent()) {
        clockStratum = 1;
        refId = NTP_REF_GPS;
        rootDelay = 0;
        rootDispersion = 0;
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
    int32_t stale = (int32_t) (CLK_MONOTONIC_INT() - server->lastActive);
    if(
            server->leapIndicator == NTP_LI_ALARM ||
            server->stratum == 0 ||
            server->stratum > NTP_MAX_STRATUM ||
            stale > NTP_ACTIVE_TIMEOUT
    ) {
        // unset reach bit if server stratum is too poor
        server->reach &= 0xFFFE;
        server->weight = 0;
        return;
    }

    float weight = 0;
    if(server->reach & 1) {
        float maxError = 0x1p-16f * (float) server->rootDispersion;
        maxError *= maxError;
        maxError = sqrtf(maxError + server->varDelay);
        maxError += server->meanDelay;
        maxError += (0x1p-16f * (float) server->rootDelay);
        weight = 1.0f / maxError;
    }
    if(!isfinite(weight))
        weight = 0;
    server->weight = weight;
}

static float toFloatU(uint64_t value) {
    uint32_t *parts = (uint32_t *) &value;
    float temp = 0x1p32f * (float) parts[1];
    temp += (float) parts[0];
    return temp;
}

static float toFloat(int64_t value) {
    if(value < 0)
        return -toFloatU((uint64_t) -value);
    else
        return toFloatU((uint64_t) value);
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
            diff += servers[i].varDelay;
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
    float _clockOffset = 0;
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
            _clockOffset += weight * servers[i].currentOffset;
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
            // mark active servers
            if(weight >= NTP_ACTIVE_THRESH)
                servers[i].lastActive = CLK_MONOTONIC_INT();
        }
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

    // update clock stats
    if(clockOffset != 0) {
        float _drift = (_clockOffset - clockOffset) / NTP_POLL_INTV;
        float _skew = _drift - clockDrift;
        // update skew
        float _clockSkew = clockSkew * clockSkew;
        _clockSkew += ((_skew * _skew) - _clockSkew) * NTP_STAT_RATE;
        clockSkew = sqrtf(_clockSkew);
        // update drift
        clockDrift += (_drift - clockDrift) * NTP_STAT_RATE;
    }
    clockOffset = _clockOffset;

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

    // skip clock correction if GPS is locked
    if(GPSDO_ppsPresent()) return;

    if (offset != 0) {
        uint32_t *parts = (uint32_t *) &offset;
        // correct truncation for negative values
        if(
            parts[1] & (1u << 31u) &&
            parts[0] & (1u << 31u)
        ) ++parts[1];
        // apply offset adjustment
        ((uint32_t *) &ntpOffset)[1] += parts[1];
    }

    // notify GPSDO of ntp status for fail-over purposes
    if(norm > 0) {
        // update GPSDO trimming and reset state if clock was hard stepped
        if (GPSDO_ntpUpdate(clockOffset, clockSkew)) {
            // reset server stats
            for (int i = 0; i < SERVER_COUNT; i++)
                resetServer(servers + i);
            // reset clock stats
            clockDrift = 0;
            clockOffset = 0;
            clockSkew = 0;
        }
    }

    // set stratum status
    if(_stratum == 0) {
        clockStratum = 16;
        rootDelay = 0;
        rootDispersion = 0;
    } else {
        clockStratum = 1 + (int) roundf(_stratum);
        rootDelay = (int32_t) roundf(0x1p16f * _delay);
        rootDispersion = (int32_t) roundf(0x1p16f * sqrtf(_dispersion));
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
    // check DHCP supplied servers first
    uint32_t *addr;
    int count;
    DHCP_ntpAddr(&addr, &count);
    for(int i = 0; i < count; i++) {
        callbackDNS(addr[i]);
    }

    // perform DNS lookup
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
    if((NTP_POLL_PING + server->pollSlot) != ntpProcess.timeSlot)
        return;
    // verify that server is configured
    if(server->addr == 0)
        return;
    // ping server in advance of poll request
    pollServer(server, 1);
}

static void requestTxCallback(uint8_t *frame, int flen) {
    // map headers
    struct FRAME_ETH *headerEth = (struct FRAME_ETH *) frame;
    struct HEADER_IPv4 *headerIPv4 = (struct HEADER_IPv4 *) (headerEth + 1);
    struct HEADER_UDP *headerUDP = (struct HEADER_UDP *) (headerIPv4 + 1);
    struct HEADER_NTPv4 *headerNTP = (struct HEADER_NTPv4 *) (headerUDP + 1);

    // store hardware transmit time
    uint32_t cell = hashXleave(headerIPv4->dst);
    tsXleave[cell].rxTime = headerNTP->rxTime;
    tsXleave[cell].txTime = __builtin_bswap64(ntpTimeTx(frame));
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
    // prevent loopback
    if(headerIPv4->src == ipAddress) return;
    // filter non-client frames
    if(headerNTP->mode != 3) return;
    // indicate time-server activity
    LED_act0();

    // retrieve rx time
    const uint64_t rxTime = ntpTimeRx(frame);

    // get TX descriptor
    const int txDesc = NET_getTxDesc();
    if(txDesc < 0) return;
    // set callback for precise TX timestamp
    NET_setTxCallback(txDesc, requestTxCallback);
    // duplicate packet for sending
    uint8_t *temp = NET_getTxBuff(txDesc);
    memcpy(temp, frame, flen);
    frame = temp;

    // return the response directly to the sender
    UDP_returnToSender(frame, ipAddress, NTP_SRV_PORT);

    // remap headers
    headerEth = (struct FRAME_ETH *) frame;
    headerIPv4 = (struct HEADER_IPv4 *) (headerEth + 1);
    headerUDP = (struct HEADER_UDP *) (headerIPv4 + 1);
    headerNTP = (struct HEADER_NTPv4 *) (headerUDP + 1);

    // check for interleaved request
    int xleave = -1;
    const uint64_t orgTime = headerNTP->origTime;
    if(
            orgTime != 0 &&
            headerNTP->rxTime != headerNTP->txTime
    ) {
        // load TX timestamp if available
        int cell = hashXleave(headerIPv4->dst);
        if(orgTime == tsXleave[cell].rxTime)
            xleave = cell;
    }

    // set type to server response
    headerNTP->mode = 4;
    headerNTP->status = leapIndicator;
    // set stratum and precision
    headerNTP->stratum = clockStratum;
    headerNTP->precision = NTP_PRECISION;
    // set root delay
    headerNTP->rootDelay = __builtin_bswap32(rootDelay);
    // set root dispersion
    headerNTP->rootDispersion = __builtin_bswap32(rootDispersion);
    // set reference ID
    headerNTP->refID = refId;
    // set reference timestamp
    headerNTP->refTime = __builtin_bswap64(ntpModified());
    // set origin and TX timestamps
    if(xleave < 0) {
        headerNTP->origTime = headerNTP->txTime;
        headerNTP->txTime = __builtin_bswap64(ntpClock());
    } else {
        headerNTP->origTime = headerNTP->rxTime;
        headerNTP->txTime = tsXleave[xleave].txTime;
    }
    // set RX timestamp
    headerNTP->rxTime = __builtin_bswap64(rxTime);

    // finalize packet
    UDP_finalize(frame, flen);
    IPv4_finalize(frame, flen);
    // transmit packet
    NET_transmit(txDesc, flen);
}

static void callbackARP(uint32_t remoteAddress, uint8_t *macAddress) {
    // special case for external addresses
    if(remoteAddress == ipGateway) {
        for(int i = 0; i < SERVER_COUNT; i++) {
            if(IPv4_testSubnet(ipSubnet, ipAddress, servers[i].addr)) {
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
    // prevent loop back
    if(addr == ipAddress) return;
    // prevent duplicate entries
    for (int i = 0; i < SERVER_COUNT; i++) {
        if(servers[i].addr == addr)
            return;
    }
    // fill empty server slot
    for (int i = 0; i < SERVER_COUNT; i++) {
        if (servers[i].addr == 0) {
            servers[i].addr = addr;
            servers[i].lastActive = CLK_MONOTONIC_INT();
            break;
        }
    }
}

static void processResponse(uint8_t *frame, int flen) {
    // discard malformed packets
    if(flen < NTP4_SIZE) return;

    // map headers
    struct FRAME_ETH *headerEth = (struct FRAME_ETH *) frame;
    struct HEADER_IPv4 *headerIPv4 = (struct HEADER_IPv4 *) (headerEth + 1);
    struct HEADER_UDP *headerUDP = (struct HEADER_UDP *) (headerIPv4 + 1);
    struct HEADER_NTPv4 *headerNTP = (struct HEADER_NTPv4 *) (headerUDP + 1);

    // verify destination
    if(headerIPv4->dst != ipAddress) return;
    // prevent loopback
    if(headerIPv4->src == ipAddress) return;
    // filter non-server frames
    if(headerNTP->mode != 4) return;

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
    stamps[3] = ntpTimeRx(frame);

    // discard responses with aberrant round-trip times
    int64_t roundTrip = (int64_t) (stamps[3] - stamps[0]);
    if(((uint32_t *) &roundTrip)[1] != 0) return;
    // discard responses with aberrant delays
    int64_t delay = ((int64_t) (roundTrip - (stamps[2] - stamps[1]))) >> 1;
    if(((uint32_t *) &delay)[1] != 0) return;

    // update sequence state
    server->prevTxSrv = headerNTP->txTime;
    server->prevRx = __builtin_bswap64(stamps[3]);

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
    uint64_t txTime = __builtin_bswap64(headerNTP->txTime);
    for(int i = 0; i < NTP_BURST; i++) {
        uint64_t *set = server->stamps + (i << 2);
        if(set[0] == txTime) {
            // update timestamp if unmodified
            if(set[1] == set[0])
                set[1] = ntpTimeTx(frame);
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
    headerUDP->portSrc = __builtin_bswap16(NTP_CLI_PORT);
    headerUDP->portDst = __builtin_bswap16(NTP_SRV_PORT);

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
    headerNTP->refTime = __builtin_bswap64(ntpModified());
    // set origin timestamp
    headerNTP->origTime = server->prevTxSrv;
    // set rx time timestamp
    headerNTP->rxTime = server->prevRx;
    // set TX time
    uint64_t txTime = ntpClock();
    headerNTP->txTime = __builtin_bswap64(txTime);

    // record TX timestamp
    if(pingOnly == 0) {
        uint64_t *stamps = server->stamps + (server->burst << 2);
        stamps[0] = txTime;
        stamps[1] = txTime;
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



// -----------------------------------------------------------------------------
// support for chronyc status queries ------------------------------------------
// -----------------------------------------------------------------------------

#define REQ_MIN_LEN (offsetof(CMD_Request, data.null.EOR))
#define REQ_MAX_LEN (sizeof(CMD_Request))

#define REP_LEN_NSOURCES (offsetof(CMD_Reply, data.n_sources.EOR))
#define REP_LEN_SOURCEDATA (offsetof(CMD_Reply, data.source_data.EOR))
#define REP_LEN_SOURCESTATS (offsetof(CMD_Reply, data.sourcestats.EOR))
#define REP_LEN_TRACKING (offsetof(CMD_Reply, data.tracking.EOR))

__attribute__((always_inline))
static inline uint16_t htons(uint16_t value) { return __builtin_bswap16(value); }
static inline uint32_t htonl(uint32_t value) { return __builtin_bswap32(value); }
static int32_t htonf(float value);

static void candmReply(CMD_Reply *cmdReply, const CMD_Request *cmdRequest);
static void processNSources(CMD_Reply *cmdReply, const CMD_Request *cmdRequest);
static void processSourceData(CMD_Reply *cmdReply, const CMD_Request *cmdRequest);
static void processSourceStats(CMD_Reply *cmdReply, const CMD_Request *cmdRequest);
static void processTracking(CMD_Reply *cmdReply, const CMD_Request *cmdRequest);

static void processChrony(uint8_t *frame, int flen) {
    // drop invalid packets
    if(flen < (UDP_DATA_OFFSET + REQ_MIN_LEN)) return;
    if(flen > (UDP_DATA_OFFSET + REQ_MAX_LEN)) return;

    // map headers
    struct FRAME_ETH *headerEth = (struct FRAME_ETH *) frame;
    struct HEADER_IPv4 *headerIPv4 = (struct HEADER_IPv4 *) (headerEth + 1);
    struct HEADER_UDP *headerUDP = (struct HEADER_UDP *) (headerIPv4 + 1);
    CMD_Request *cmdRequest = (CMD_Request *) (headerUDP + 1);

    // drop invalid packets
    if(cmdRequest->pkt_type != PKT_TYPE_CMD_REQUEST) return;
    if(cmdRequest->pad1 != 0) return;
    if(cmdRequest->pad2 != 0) return;

    // drop packet if nack is not possible
    if(cmdRequest->version != PROTO_VERSION_NUMBER) {
        if(cmdRequest->version < PROTO_VERSION_MISMATCH_COMPAT_SERVER)
            return;
    }

    int txDesc = NET_getTxDesc();
    if(txDesc < 0) return;
    // allocate and clear frame buffer
    uint8_t *resp = NET_getTxBuff(txDesc);
    memset(resp, 0, UDP_DATA_OFFSET + sizeof(CMD_Reply));
    // return to sender
    memcpy(resp, frame, UDP_DATA_OFFSET);
    UDP_returnToSender(resp, ipAddress, DEFAULT_CANDM_PORT);

    // remap headers
    headerEth = (struct FRAME_ETH *) resp;
    headerIPv4 = (struct HEADER_IPv4 *) (headerEth + 1);
    headerUDP = (struct HEADER_UDP *) (headerIPv4 + 1);
    CMD_Reply *cmdReply = (CMD_Reply *) (headerUDP + 1);

    // begin response
    candmReply(cmdReply, cmdRequest);

    // nack bad protocol version
    if(cmdRequest->version != PROTO_VERSION_NUMBER) {
        cmdReply->status = htons(STT_BADPKTVERSION);
    } else {
        uint16_t cmd = htons(cmdRequest->command);
        int len = flen - UDP_DATA_OFFSET;
        if (cmd == REQ_N_SOURCES) {
            if(len == REP_LEN_NSOURCES)
                processNSources(cmdReply, cmdRequest);
            else
                cmdReply->status = htons(STT_BADPKTLENGTH);
        }
        else if (cmd == REQ_SOURCE_DATA) {
            if(len == REP_LEN_SOURCEDATA)
                processSourceData(cmdReply, cmdRequest);
            else
                cmdReply->status = htons(STT_BADPKTLENGTH);
        }
        else if (cmd == REQ_SOURCESTATS) {
            if(len == REP_LEN_SOURCESTATS)
                processSourceStats(cmdReply, cmdRequest);
            else
                cmdReply->status = htons(STT_BADPKTLENGTH);
        }
        else if (cmd == REQ_TRACKING) {
            if(len == REP_LEN_TRACKING)
                processTracking(cmdReply, cmdRequest);
            else
                cmdReply->status = htons(STT_BADPKTLENGTH);
        }
        else {
            cmdReply->status = htons(STT_INVALID);
        }
    }

    // finalize packet
    UDP_finalize(resp, flen);
    IPv4_finalize(resp, flen);
    // transmit packet
    NET_transmit(txDesc, flen);
}

static void candmReply(CMD_Reply *cmdReply, const CMD_Request *cmdRequest) {
    cmdReply->version = PROTO_VERSION_NUMBER;
    cmdReply->pkt_type = PKT_TYPE_CMD_REPLY;
    cmdReply->command = cmdRequest->command;
    cmdReply->reply = htons(RPY_NULL);
    cmdReply->status = htons(STT_SUCCESS);
    cmdReply->sequence = cmdRequest->sequence;
}

static void processNSources(CMD_Reply *cmdReply, const CMD_Request *cmdRequest) {
    cmdReply->reply = htons(RPY_N_SOURCES);
    cmdReply->data.n_sources.n_sources = htonl(SERVER_COUNT);
}

static void processSourceData(CMD_Reply *cmdReply, const CMD_Request *cmdRequest) {
    cmdReply->reply = htons(RPY_SOURCE_DATA);

    uint32_t i = htonl(cmdRequest->data.source_data.index);
    if(i >= SERVER_COUNT) return;

    struct Server *server = servers + i;
    cmdReply->data.source_data.ip_addr.family = htons(IPADDR_INET4);
    cmdReply->data.source_data.ip_addr.addr.in4 = server->addr;
    cmdReply->data.source_data.stratum = htons(server->stratum);
    cmdReply->data.source_data.reachability = htons(server->reach & 0xFF);
    cmdReply->data.source_data.orig_latest_meas.f = htonf(server->currentOffset);
    cmdReply->data.source_data.latest_meas.f = htonf(server->currentOffset);
    cmdReply->data.source_data.latest_meas_err.f = htonf(server->meanDelay + sqrtf(server->varDelay));
    cmdReply->data.source_data.since_sample = htonl((ntpClock() - server->update) >> 32);
    cmdReply->data.source_data.poll = (int16_t) htons(NTP_POLL_BITS);
    if(server->weight > 0.01f) {
        cmdReply->data.source_data.state = htons(RPY_SD_ST_SELECTED);
    }
    else if(server->weight != 0) {
        cmdReply->data.source_data.state = htons(RPY_SD_ST_SELECTABLE);
    }
    else {
        cmdReply->data.source_data.state = htons(RPY_SD_ST_UNSELECTED);
    }
}

static void processSourceStats(CMD_Reply *cmdReply, const CMD_Request *cmdRequest) {
    cmdReply->reply = htons(RPY_SOURCESTATS);

    uint32_t i = htonl(cmdRequest->data.source_data.index);
    if(i >= SERVER_COUNT) return;

    struct Server *server = servers + i;
    cmdReply->data.sourcestats.ip_addr.family = htons(IPADDR_INET4);
    cmdReply->data.sourcestats.ip_addr.addr.in4 = server->addr;
    cmdReply->data.sourcestats.est_offset.f = htonf(server->currentOffset);
    cmdReply->data.sourcestats.est_offset_err.f = htonf(sqrtf(server->varDelay));
    cmdReply->data.sourcestats.resid_freq_ppm.f = htonf(server->meanDrift * 1e6f);
    cmdReply->data.sourcestats.skew_ppm.f = htonf(sqrtf(server->varDrift) * 1e6f);
}

static void processTracking(CMD_Reply *cmdReply, const CMD_Request *cmdRequest) {
    union fixed_32_32 scratch;

    cmdReply->reply = htons(RPY_TRACKING);
    cmdReply->data.tracking.ref_id = refId;
    cmdReply->data.tracking.stratum = htons(clockStratum);
    cmdReply->data.tracking.leap_status = htons(leapIndicator);
    if(refId == NTP_REF_GPS) {
        cmdReply->data.tracking.ip_addr.family = htons(IPADDR_ID);
        cmdReply->data.tracking.ip_addr.addr.id = refId;
    } else {
        cmdReply->data.tracking.ip_addr.family = htons(IPADDR_INET4);
        cmdReply->data.tracking.ip_addr.addr.in4 = refId;
    }

    scratch.full = ntpModified();
    cmdReply->data.tracking.ref_time.tv_sec_high = 0;
    cmdReply->data.tracking.ref_time.tv_sec_low = htonl(scratch.ipart - NTP_UTC_OFFSET);
    cmdReply->data.tracking.ref_time.tv_nsec = htonl(scratch.fpart);

    cmdReply->data.tracking.current_correction.f = htonf(-GPSDO_offsetMean());
    cmdReply->data.tracking.last_offset.f = htonf(-1e-9f * (float) GPSDO_offsetNano());
    cmdReply->data.tracking.rms_offset.f = htonf(GPSDO_offsetRms());

    cmdReply->data.tracking.freq_ppm.f = htonf(GPSDO_compValue() * 1e6f);
    cmdReply->data.tracking.resid_freq_ppm.f = htonf(GPSDO_pllTrim() * 1e6f);
    cmdReply->data.tracking.skew_ppm.f = htonf(GPSDO_skewRms() * 1e6f);

    cmdReply->data.tracking.root_delay.f = htonf(0x1p-16f * (float) rootDelay);
    cmdReply->data.tracking.root_dispersion.f = htonf(0x1p-16f * (float) rootDispersion);

    cmdReply->data.tracking.last_update_interval.f = htonf((clockStratum == 1) ? 1 : 64);
}


/**
 * convert IEEE 754 to candm float format
 * @param value IEEE 754 single-precision float
 * @return candm float format
 */
static int32_t htonf(float value) {
    // decompose IEEE 754 single-precision float
    uint32_t raw = *(uint32_t*) &value;
    uint32_t coef = raw & ((1 << 23) - 1);
    int32_t exp = ((int32_t) (raw >> 23 & 0xFF)) - 127;
    uint32_t sign = (raw >> 31) & 1;

    // small values and NaNs
    if(exp < -65 || (exp == 128 && coef != 0))
        return 0;

    // large values
    if(exp > 61)
        return (int32_t) (sign ? 0x0000007Fu : 0xFFFFFF7Eu);

    // normal values
    coef |= 1 << 23;
    if(sign) coef = -coef;
    coef &= (1 << 25) - 1;
    return (int32_t) htonl(coef | ((exp + 2) << 25));
}
