//
// Created by robert on 4/27/23.
//

#include <memory.h>
#include <math.h>

#include "../chrony/candm.h"
#include "../clk/tai.h"
#include "../clk/comp.h"
#include "../clk/mono.h"
#include "../clk/util.h"
#include "../led.h"
#include "../net.h"
#include "../net/dhcp.h"
#include "../net/dns.h"
#include "../net/eth.h"

#include "common.h"
#include "ntp.h"
#include "peer.h"
#include "ref.h"

#define NTP_TS_PREC (-24)
#define MAX_NTP_PEERS (8)
#define MAX_NTP_SRCS (9)
#define MIN_DNS_INTV (16) // 16 seconds

#define NTP_POOL_FQDN ("pool.ntp.org")

static NtpGPS srcGps;
static NtpPeer peerSlots[MAX_NTP_PEERS];
// allocate new peer
static NtpSource* ntpAllocPeer();
// deallocate peer
static void ntpDeallocatePeer(NtpSource *peer);

static NtpSource * volatile sources[MAX_NTP_SRCS];
static volatile uint32_t cntSources = 0;
static volatile uint32_t runNext = 0;
static volatile uint32_t activeSource = 0;
static volatile uint64_t lastUpdate = 0;

static volatile uint32_t lastDnsRequest = 0;

// aggregate NTP state
static volatile int leapIndicator;
static volatile int clockStratum = 16;
static volatile uint32_t refId;
static volatile uint32_t rootDelay;
static volatile uint32_t rootDispersion;


// hash table for interleaved timestamps
#define XLEAVE_COUNT (1024)
static volatile struct TsXleave {
    uint64_t rxTime;
    uint64_t txTime;
} tsXleave[XLEAVE_COUNT];
// hash function for interleaved timestamp table
static inline int hashXleave(uint32_t addr) {
    return (int) (((addr * 0xDE9DB139) >> 22) & (XLEAVE_COUNT - 1));
}

// request handlers
static void ntpRequest(uint8_t *frame, int flen);
static void chronycRequest(uint8_t *frame, int flen);
// response handlers
static void ntpResponse(uint8_t *frame, int flen);

// internal operations
static void ntpMain();
static void ntpDnsCallback(void *ref, uint32_t addr);

// timestamp convenience functions
static inline uint64_t ntpClock() { return NTP_UTC_OFFSET + CLK_TAI() - clkTaiUtcOffset; }
static inline uint64_t ntpModified() { return 0; }


void NTP_init() {
    UDP_register(NTP_PORT_SRV, ntpRequest);
    UDP_register(NTP_PORT_CLI, ntpResponse);
    // listen for crony status requests
    UDP_register(DEFAULT_CANDM_PORT, chronycRequest);

    // initialize empty peer records
    for(int i = 0; i < MAX_NTP_PEERS; i++)
        NtpPeer_init(peerSlots + i);

    // initialize GPS reference and register it as a source
    NtpGPS_init(&srcGps);
    sources[cntSources++] = (void *) &srcGps;
}

void NTP_run() {
    if(runNext < cntSources) {
        NtpSource *source = sources[runNext++];
        (*(source->run))(source);
    } else {
        ntpMain();
        runNext = 0;
    }
}

static void ntpTxCallback(void *ref, uint8_t *frame, int flen) {
    // map headers
    struct FRAME_ETH *headerEth = (struct FRAME_ETH *) frame;
    struct HEADER_IPv4 *headerIPv4 = (struct HEADER_IPv4 *) (headerEth + 1);
    struct HEADER_UDP *headerUDP = (struct HEADER_UDP *) (headerIPv4 + 1);
    struct HEADER_NTPv4 *headerNTP = (struct HEADER_NTPv4 *) (headerUDP + 1);

    // retrieve hardware transmit time
    uint64_t stamps[3];
    NET_getTxTime(frame, stamps);
    const uint64_t txTime = (stamps[2] - clkTaiUtcOffset) + NTP_UTC_OFFSET;

    // record hardware transmit time
    uint32_t cell = hashXleave(headerIPv4->dst);
    tsXleave[cell].rxTime = headerNTP->rxTime;
    tsXleave[cell].txTime = __builtin_bswap64(txTime);
}

// process client request
static void ntpRequest(uint8_t *frame, int flen) {
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
    if(headerNTP->mode != NTP_MODE_CLI) return;
    // indicate time-server activity
    LED_act0();

    // retrieve rx time
    uint64_t stamps[3];
    NET_getRxTime(frame, stamps);
    // translate TAI timestamp into NTP domain
    const uint64_t rxTime = (stamps[2] - clkTaiUtcOffset) + NTP_UTC_OFFSET;

    // get TX descriptor
    const int txDesc = NET_getTxDesc();
    if(txDesc < 0) return;
    // set callback for precise TX timestamp
    NET_setTxCallback(txDesc, ntpTxCallback, NULL);
    // duplicate packet for sending
    uint8_t *temp = NET_getTxBuff(txDesc);
    memcpy(temp, frame, flen);
    frame = temp;

    // return the response directly to the sender
    UDP_returnToSender(frame, ipAddress, NTP_PORT_SRV);

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
    headerNTP->mode = NTP_MODE_SRV;
    headerNTP->status = leapIndicator;
    // set stratum and precision
    headerNTP->stratum = clockStratum;
    headerNTP->precision = NTP_TS_PREC;
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

// process peer response
static void ntpResponse(uint8_t *frame, int flen) {
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
    if(headerNTP->mode != NTP_MODE_SRV) return;

    // locate recipient
    uint32_t srcAddr = headerIPv4->src;
    for(uint32_t i = 0; i < cntSources; i++) {
        volatile struct NtpSource *source = sources[i];

        // check for matching peer
        if(source == NULL) return;
        if(source->mode != RPY_SD_MD_CLIENT) continue;
        if(source->id != srcAddr) continue;

        // handoff frame to peer
        NtpPeer_recv(source, frame, flen);
        return;
    }
}

static void ntpMain() {
    // periodically attempt to fill empty source slots
    uint32_t now = CLK_MONO_INT();
    if ((now - lastDnsRequest) > MIN_DNS_INTV) {
        lastDnsRequest = now;

        if(cntSources < MAX_NTP_SRCS) {
            // fill with DHCP provided addresses
            uint32_t *addr;
            int cnt;
            DHCP_ntpAddr(&addr, &cnt);
            for (int i = 0; i < cnt; i++)
                ntpDnsCallback(NULL, addr[i]);

            // fill with servers from public ntp pool
            DNS_lookup(NTP_POOL_FQDN, ntpDnsCallback, NULL);
        }
    }

    // select best clock
    activeSource = 0;
    float score = sources[0]->score;

    NtpSource *source = sources[activeSource];
    if(source == NULL) return;
    if(source->lastUpdate == lastUpdate) return;
    lastUpdate = source->lastUpdate;
    if(source->used < 8) return;

    // adjust frequency compensation
    if(source->freqSkew < 5e-6f) {
        int32_t comp = CLK_COMP_getComp();
        comp += (int32_t) (0x1p32f * 0x1p-6f * source->freqDrift);
        CLK_COMP_setComp(comp);
    }

    // adjust TAI alignment
    if(source->offsetMean > 0.25f) {
        CLK_TAI_adjust(source->pollSample[source->samplePtr].offset);
    }
    else {
//        CLK_TAI_setTrim(-(int32_t) (0x1p32f * 0x1p-8f * source->offsetMean));
        if(fabsf(source->offsetMean) > 100e-6f)
            CLK_TAI_align((int32_t) (0x1p32f * source->offsetMean));
        else
            CLK_TAI_align((int32_t) (0x1p32f * 0x1p-6f * source->offsetMean));
    }
}

volatile static struct NtpSource* ntpAllocPeer() {
    for(int i = 0; i < MAX_NTP_PEERS; i++) {
        NtpPeer *slot = peerSlots + i;
        if(slot->source.id == 0) {
            // initialize peer record
            (*(slot->source.init))(slot);
            // append to source list
            sources[cntSources++] = (struct NtpSource *) slot;
            // return instance
            return (struct NtpSource *) slot;
        }
    }
    return NULL;
}

static void ntpDeallocatePeer(NtpSource *peer) {
    peer->id = 0;
    uint32_t slot = -1u;
    for(uint32_t i = 0; i < cntSources; i++) {
        if(sources[i] == peer) slot = i;
    }
    if(slot > cntSources) return;
    // remove source from source list
    --cntSources;
    for(uint32_t i = slot; i < cntSources; i++)
        sources[i] = sources[i+1];
    // clear empty slots
    for(uint32_t i = cntSources; i < MAX_NTP_SRCS; i++)
        sources[i] = NULL;
}

static void ntpDnsCallback(void *ref, uint32_t addr) {
    // ignore if slots are full
    if(cntSources >= MAX_NTP_SRCS) return;
    // ignore own address
    if(addr == ipAddress) return;
    // verify address is not currently in use
    for(uint32_t i = 0; i < cntSources; i++) {
        if(sources[i]->mode != RPY_SD_MD_CLIENT)
            continue;
        if(sources[i]->id == addr)
            return;
    }
    // attempt to add as new source
    NtpSource *newSource = ntpAllocPeer();
    if(newSource != NULL)
        newSource->id = addr;
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

// replicate htons() function
__attribute__((always_inline))
static inline uint16_t htons(uint16_t value) { return __builtin_bswap16(value); }

// replicate htonl() function
__attribute__((always_inline))
static inline uint32_t htonl(uint32_t value) { return __builtin_bswap32(value); }

// replicate chronyc float format
static int32_t htonf(float value);

// begin chronyc reply
static void chronycReply(CMD_Reply *cmdReply, const CMD_Request *cmdRequest);

// chronyc command handlers
static uint16_t chronycNSources(CMD_Reply *cmdReply, const CMD_Request *cmdRequest);
static uint16_t chronycSourceData(CMD_Reply *cmdReply, const CMD_Request *cmdRequest);
static uint16_t chronycSourceStats(CMD_Reply *cmdReply, const CMD_Request *cmdRequest);
static uint16_t chronycTracking(CMD_Reply *cmdReply, const CMD_Request *cmdRequest);

#define CHRONYC_HANDLER_CNT (4)
static const struct {
    uint16_t (*call) (CMD_Reply *cmdReply, const CMD_Request *cmdRequest);
    int len;
    uint16_t cmd;
} handlers[CHRONYC_HANDLER_CNT] = {
        { chronycNSources, REP_LEN_NSOURCES, REQ_N_SOURCES },
        { chronycSourceData, REP_LEN_SOURCEDATA, REQ_SOURCE_DATA },
        { chronycSourceStats, REP_LEN_SOURCESTATS, REQ_SOURCESTATS },
        { chronycTracking, REP_LEN_TRACKING, REQ_TRACKING }
};

static void chronycRequest(uint8_t *frame, int flen) {
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
    chronycReply(cmdReply, cmdRequest);

    // nack bad protocol version
    if(cmdRequest->version != PROTO_VERSION_NUMBER) {
        cmdReply->status = htons(STT_BADPKTVERSION);
    } else {
        cmdReply->status = htons(STT_INVALID);
        const uint16_t cmd = htons(cmdRequest->command);
        const int len = flen - UDP_DATA_OFFSET;
        for(int i = 0; i < CHRONYC_HANDLER_CNT; i++) {
            if(handlers[i].cmd == cmd) {
                if(handlers[i].len == len)
                    cmdReply->status = (*(handlers[i].call))(cmdReply, cmdRequest);
                else
                    cmdReply->status = htons(STT_BADPKTLENGTH);
                break;
            }
        }
    }

    // finalize packet
    UDP_finalize(resp, flen);
    IPv4_finalize(resp, flen);
    // transmit packet
    NET_transmit(txDesc, flen);
}

static void chronycReply(CMD_Reply *cmdReply, const CMD_Request *cmdRequest) {
    cmdReply->version = PROTO_VERSION_NUMBER;
    cmdReply->pkt_type = PKT_TYPE_CMD_REPLY;
    cmdReply->command = cmdRequest->command;
    cmdReply->reply = htons(RPY_NULL);
    cmdReply->status = htons(STT_SUCCESS);
    cmdReply->sequence = cmdRequest->sequence;
}

static uint16_t chronycNSources(CMD_Reply *cmdReply, const CMD_Request *cmdRequest) {
    cmdReply->reply = htons(RPY_N_SOURCES);
    cmdReply->data.n_sources.n_sources = htonl(cntSources);
    return htons(STT_SUCCESS);
}

static uint16_t chronycSourceData(CMD_Reply *cmdReply, const CMD_Request *cmdRequest) {
    cmdReply->reply = htons(RPY_SOURCE_DATA);
    // locate source
    const uint32_t i = htonl(cmdRequest->data.source_data.index);
    if(i >= cntSources) return htons(STT_NOSUCHSOURCE);
    // sanity check
    NtpSource *source = sources[i];
    if(source == NULL) return htons(STT_NOSUCHSOURCE);

    // set source id
    cmdReply->data.source_data.mode = htons(source->mode);
    cmdReply->data.source_data.ip_addr.family = htons(IPADDR_INET4);
    cmdReply->data.source_data.ip_addr.addr.in4 = source->id;

    cmdReply->data.source_data.stratum = htons(source->stratum);
    cmdReply->data.source_data.reachability = htons(source->reach & 0xFF);
    cmdReply->data.source_data.orig_latest_meas.f = htonf(source->lastOffsetOrig);
    cmdReply->data.source_data.latest_meas.f = htonf(source->lastOffset);
    cmdReply->data.source_data.latest_meas_err.f = htonf(source->lastDelay);
    cmdReply->data.source_data.since_sample = htonl((CLK_MONO() - source->lastUpdate) >> 32);
    cmdReply->data.source_data.poll = (int16_t) htons(source->poll);
    cmdReply->data.source_data.state = htons(source->mode);
//    if(server->weight > 0.01f)
//        cmdReply->data.source_data.state = htons(RPY_SD_ST_SELECTED);
//    else if(server->weight > 0)
//        cmdReply->data.source_data.state = htons(RPY_SD_ST_SELECTABLE);
//    else
//        cmdReply->data.source_data.state = htons(RPY_SD_ST_UNSELECTED);
    return htons(STT_SUCCESS);
}

static uint16_t chronycSourceStats(CMD_Reply *cmdReply, const CMD_Request *cmdRequest) {
    cmdReply->reply = htons(RPY_SOURCESTATS);

    const uint32_t i = htonl(cmdRequest->data.sourcestats.index);
    if(i >= cntSources) return htons(STT_NOSUCHSOURCE);
    // sanity check
    NtpSource *source = sources[i];
    if(source == NULL) return htons(STT_NOSUCHSOURCE);

    // set source id
    if(source->mode == RPY_SD_MD_REF) {
        cmdReply->data.sourcestats.ref_id = source->id;
        cmdReply->data.sourcestats.ip_addr.family = htons(IPADDR_UNSPEC);
        cmdReply->data.sourcestats.ip_addr.addr.in4 = source->id;
    }
    else {
        cmdReply->data.sourcestats.ref_id = source->ref_id;
        cmdReply->data.sourcestats.ip_addr.family = htons(IPADDR_INET4);
        cmdReply->data.sourcestats.ip_addr.addr.in4 = source->id;
    }

    cmdReply->data.sourcestats.n_samples = htonl(source->sampleCount);
    cmdReply->data.sourcestats.n_runs = htonl(source->used);
    cmdReply->data.sourcestats.span_seconds = htonl(source->span);
    cmdReply->data.sourcestats.est_offset.f = htonf(source->offsetMean);
    cmdReply->data.sourcestats.est_offset_err.f = htonf(source->offsetStdDev);
    cmdReply->data.sourcestats.resid_freq_ppm.f = htonf(source->freqDrift * 1e6f);
    cmdReply->data.sourcestats.skew_ppm.f = htonf(source->freqSkew * 1e6f);
    return htons(STT_SUCCESS);
}

static uint16_t chronycTracking(CMD_Reply *cmdReply, const CMD_Request *cmdRequest) {
    union fixed_32_32 scratch;

    cmdReply->reply = htons(RPY_TRACKING);
    cmdReply->data.tracking.ref_id = refId;
    cmdReply->data.tracking.stratum = htons(clockStratum);
    cmdReply->data.tracking.leap_status = htons(leapIndicator);
    if(refId == srcGps.source.id) {
        cmdReply->data.tracking.ip_addr.family = htons(IPADDR_UNSPEC);
        cmdReply->data.tracking.ip_addr.addr.in4 = refId;
    } else {
        cmdReply->data.tracking.ip_addr.family = htons(IPADDR_INET4);
        cmdReply->data.tracking.ip_addr.addr.in4 = refId;
    }

//    scratch.full = ntpModified();
//    cmdReply->data.tracking.ref_time.tv_sec_high = 0;
//    cmdReply->data.tracking.ref_time.tv_sec_low = htonl(scratch.ipart - NTP_UTC_OFFSET);
//    cmdReply->data.tracking.ref_time.tv_nsec = htonl(scratch.fpart);

//    cmdReply->data.tracking.current_correction.f = htonf(-GPSDO_offsetMean());
//    cmdReply->data.tracking.last_offset.f = htonf(-1e-9f * (float) GPSDO_offsetNano());
//    cmdReply->data.tracking.rms_offset.f = htonf(GPSDO_offsetRms());

    cmdReply->data.tracking.freq_ppm.f = htonf(-1e6f * (0x1p-32f * (float) CLK_COMP_getComp()));
    cmdReply->data.tracking.resid_freq_ppm.f = htonf(-1e6f * (0x1p-32f * (float) CLK_TAI_getTrim()));
//    cmdReply->data.tracking.skew_ppm.f = htonf(GPSDO_skewRms() * 1e6f);

    cmdReply->data.tracking.root_delay.f = htonf(0x1p-16f * (float) rootDelay);
    cmdReply->data.tracking.root_dispersion.f = htonf(0x1p-16f * (float) rootDispersion);

//    cmdReply->data.tracking.last_update_interval.f = htonf((clockStratum == 1) ? 1 : 64);
    return htons(STT_SUCCESS);
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
