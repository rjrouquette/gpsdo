//
// Created by robert on 4/27/23.
//

#include <memory.h>

#include "../chrony/candm.h"
#include "../clk/tai.h"
#include "../clk/comp.h"
#include "../clk/util.h"
#include "../net.h"
#include "../net/eth.h"
#include "../net/ip.h"
#include "../net/udp.h"

#include "ntp.h"
#include "peer.h"
#include "ref.h"

#define NTP_SRV_PORT (123)
#define NTP_CLI_PORT (12345)
#define MAX_NTP_PEERS (8)
#define MAX_NTP_SRCS (9)

static struct NtpGPS srcGps;
static struct NtpPeer peerSlots[MAX_NTP_PEERS];

static struct NtpSource *sources[MAX_NTP_SRCS];
static int cntSources = 0;
static int runNext = 0;

static void ntpRequest(uint8_t *frame, int flen);
static void ntpResponse(uint8_t *frame, int flen);
static void chronycRequest(uint8_t *frame, int flen);

static void ntpRun();

void NTP_init() {
    UDP_register(NTP_SRV_PORT, ntpRequest);
    UDP_register(NTP_CLI_PORT, ntpResponse);
    // listen for crony status requests
    UDP_register(DEFAULT_CANDM_PORT, chronycRequest);

    // add GPS reference as source
    NtpGPS_init(&srcGps);
    sources[cntSources++] = (void *) &srcGps;
}

void NTP_run() {
    if(runNext < cntSources) {
        struct NtpSource *source = sources[runNext++];
        (*(source->run))(source);
    } else {
        ntpRun();
        runNext = 0;
    }
}

static void ntpRequest(uint8_t *frame, int flen) {

}

static void ntpResponse(uint8_t *frame, int flen) {

}

static void ntpRun() {

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

static void chronycReply(CMD_Reply *cmdReply, const CMD_Request *cmdRequest);

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
        { chronycTracking, REP_LEN_TRACKING, REQ_TRACKING },
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
    uint32_t i = htonl(cmdRequest->data.source_data.index);
    if(i >= cntSources) return htons(STT_NOSUCHSOURCE);
    // sanity check
    struct NtpSource *source = sources[i];
    if(source == NULL) return htons(STT_NOSUCHSOURCE);

    // set source id
    cmdReply->data.source_data.mode = htons(source->mode);
    cmdReply->data.source_data.ip_addr.family = htons(IPADDR_INET4);
    cmdReply->data.source_data.ip_addr.addr.in4 = source->id;

    cmdReply->data.source_data.stratum = htons(source->stratum);
    cmdReply->data.source_data.reachability = htons(source->reach & 0xFF);
    cmdReply->data.source_data.orig_latest_meas.f = htonf(source->lastOffsetOrig);
    cmdReply->data.source_data.latest_meas.f = htonf(source->lastOffset);
//    cmdReply->data.source_data.latest_meas_err.f = htonf(server->meanDelay + sqrtf(server->varDelay));
//    cmdReply->data.source_data.since_sample = htonl((ntpClock() - source->update) >> 32);
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

    uint32_t i = htonl(cmdRequest->data.sourcestats.index);
    if(i >= cntSources) return htons(STT_NOSUCHSOURCE);
    // sanity check
    struct NtpSource *source = sources[i];
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
//    cmdReply->data.sourcestats.est_offset.f = htonf(server->currentOffset);
//    cmdReply->data.sourcestats.est_offset_err.f = htonf(sqrtf(server->varDelay));
    cmdReply->data.sourcestats.resid_freq_ppm.f = htonf(source->freqDrift);
    cmdReply->data.sourcestats.skew_ppm.f = htonf(source->freqSkew);
    return htons(STT_SUCCESS);
}

static uint16_t chronycTracking(CMD_Reply *cmdReply, const CMD_Request *cmdRequest) {
//    union fixed_32_32 scratch;

    cmdReply->reply = htons(RPY_TRACKING);
//    cmdReply->data.tracking.ref_id = refId;
//    cmdReply->data.tracking.stratum = htons(clockStratum);
//    cmdReply->data.tracking.leap_status = htons(leapIndicator);
//    if(refId == NTP_REF_GPS) {
//        cmdReply->data.tracking.ip_addr.family = htons(IPADDR_ID);
//        cmdReply->data.tracking.ip_addr.addr.id = refId;
//    } else {
//        cmdReply->data.tracking.ip_addr.family = htons(IPADDR_INET4);
//        cmdReply->data.tracking.ip_addr.addr.in4 = refId;
//    }

//    scratch.full = ntpModified();
//    cmdReply->data.tracking.ref_time.tv_sec_high = 0;
//    cmdReply->data.tracking.ref_time.tv_sec_low = htonl(scratch.ipart - NTP_UTC_OFFSET);
//    cmdReply->data.tracking.ref_time.tv_nsec = htonl(scratch.fpart);

//    cmdReply->data.tracking.current_correction.f = htonf(-GPSDO_offsetMean());
//    cmdReply->data.tracking.last_offset.f = htonf(-1e-9f * (float) GPSDO_offsetNano());
//    cmdReply->data.tracking.rms_offset.f = htonf(GPSDO_offsetRms());

    cmdReply->data.tracking.freq_ppm.f = htonf(1e6f * (0x1p-32f * (float) CLK_COMP_getComp()));
    cmdReply->data.tracking.resid_freq_ppm.f = htonf(1e6f * (0x1p-32f * (float) CLK_TAI_getTrim()));
//    cmdReply->data.tracking.skew_ppm.f = htonf(GPSDO_skewRms() * 1e6f);

//    cmdReply->data.tracking.root_delay.f = htonf(0x1p-16f * (float) rootDelay);
//    cmdReply->data.tracking.root_dispersion.f = htonf(0x1p-16f * (float) rootDispersion);

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
