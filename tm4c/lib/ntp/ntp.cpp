//
// Created by robert on 4/27/23.
//

#include "ntp.h"

#include "common.hpp"
#include "gps.hpp"
#include "peer.hpp"
#include "pll.h"

#include "../led.h"
#include "../net.h"
#include "../run.h"
#include "../chrony/candm.h"
#include "../chrony/util.hpp"
#include "../clk/comp.h"
#include "../clk/tai.h"
#include "../net/dhcp.h"
#include "../net/dns.h"
#include "../net/util.h"

#include <memory>

#define MAX_NTP_PEERS (8)
#define MAX_NTP_SRCS (9)

#define NTP_POOL_FQDN ("pool.ntp.org")

#define DNS_UPDT_INTV (RUN_SEC << 4)    // every 16 seconds
#define SRC_UPDT_INTV (RUN_SEC >> 4)    // 16 Hz

// static allocation for GPS source
static char rawGps[sizeof(ntp::GPS)] alignas(ntp::GPS);
// static allocation for Peers
static char rawPeers[sizeof(ntp::Peer) * MAX_NTP_PEERS] alignas(ntp::Peer);
// allocate new peer
static ntp::Source* newPeer(uint32_t ipAddr);
// deallocate peer
static void deletePeer(ntp::Source *peer);

static ntp::Source *sources[MAX_NTP_SRCS];
static ntp::Source *selectedSource = nullptr;
static volatile uint32_t cntSources = 0;
static volatile uint64_t lastUpdate = 0;

// aggregate NTP state
static volatile int leapIndicator;
static volatile int clockStratum = 16;
static volatile uint32_t refId;
static volatile uint32_t rootDelay;
static volatile uint32_t rootDispersion;


// hash table for interleaved timestamps
#define XLEAVE_SIZE (10)

static volatile struct TsXleave {
    uint64_t rxTime;
    uint64_t txTime;
} tsXleave[1u << XLEAVE_SIZE];

// hash function for interleaved timestamp table
inline auto hashXleave(const uint32_t addr) {
    return static_cast<int>((addr * 0xDE9DB139u) >> (32 - XLEAVE_SIZE));
}


// request handlers
static void ntpRequest(uint8_t *frame, int flen);
static void chronycRequest(uint8_t *frame, int flen);
// response handlers
static void ntpResponse(uint8_t *frame, int flen);

// internal operations
static void runSelect(void *ref);
static void runDnsFill(void *ref);

// called by PLL for hard TAI adjustments
extern "C" void ntpApplyOffset(const int64_t offset) {
    for (uint32_t i = 0; i < cntSources; i++) {
        if (sources[i] != nullptr)
            sources[i]->applyOffset(offset);
    }
}

void NTP_init() {
    PLL_init();

    UDP_register(NTP_PORT_SRV, ntpRequest);
    UDP_register(NTP_PORT_CLI, ntpResponse);
    // listen for crony status requests
    UDP_register(DEFAULT_CANDM_PORT, chronycRequest);

    // initialize GPS reference and register it as a source
    auto srcGps = reinterpret_cast<ntp::GPS*>(rawGps);
    sources[cntSources++] = new(srcGps) ntp::GPS();

    // update source selection at 16 Hz
    runSleep(SRC_UPDT_INTV, runSelect, nullptr);
    // fill empty slots every 16 seconds
    runSleep(DNS_UPDT_INTV, runDnsFill, nullptr);
}

uint32_t NTP_refId() {
    return refId;
}

static void ntpTxCallback(void *ref, uint8_t *frame, int flen) {
    // map headers
    auto &packet = PacketUDP<HEADER_NTP>::from(frame);

    // retrieve hardware transmit time
    uint64_t stamps[3];
    NET_getTxTime(frame, stamps);
    const uint64_t txTime = (stamps[2] - clkTaiUtcOffset) + NTP_UTC_OFFSET;

    // record hardware transmit time
    const uint32_t cell = hashXleave(packet.ip4.dst);
    tsXleave[cell].rxTime = packet.data.rxTime;
    tsXleave[cell].txTime = htonll(txTime);
}

// process client request
static void ntpRequest(uint8_t *frame, const int flen) {
    // discard malformed packets
    if (flen < NTP4_SIZE)
        return;
    // map headers
    const auto &request = PacketUDP<HEADER_NTP>::from(frame);

    // verify destination
    if (request.ip4.dst != ipAddress)
        return;
    // prevent loopback
    if (request.ip4.src == ipAddress)
        return;
    // filter non-client frames
    if (request.data.mode != NTP_MODE_CLI)
        return;
    // indicate time-server activity
    LED_act0();

    // retrieve rx time
    uint64_t stamps[3];
    NET_getRxTime(frame, stamps);
    // translate TAI timestamp into NTP domain
    const uint64_t rxTime = (stamps[2] - clkTaiUtcOffset) + NTP_UTC_OFFSET;

    // get TX descriptor
    const int txDesc = NET_getTxDesc();
    // set callback for precise TX timestamp
    NET_setTxCallback(txDesc, ntpTxCallback, nullptr);
    // duplicate packet for sending
    uint8_t *temp = NET_getTxBuff(txDesc);
    memcpy(temp, frame, flen);
    frame = temp;

    // return the response directly to the sender
    UDP_returnToSender(frame, ipAddress, NTP_PORT_SRV);

    // map headers
    auto &response = PacketUDP<HEADER_NTP>::from(frame);

    // check for interleaved request
    int xleave = -1;
    const uint64_t orgTime = response.data.origTime;
    if (
        orgTime != 0 &&
        response.data.rxTime != response.data.txTime
    ) {
        // load TX timestamp if available
        const int cell = hashXleave(response.ip4.dst);
        if (orgTime == tsXleave[cell].rxTime)
            xleave = cell;
    }

    // set type to server response
    response.data.mode = NTP_MODE_SRV;
    response.data.status = leapIndicator;
    // set stratum and precision
    response.data.stratum = clockStratum;
    response.data.precision = NTP_CLK_PREC;
    // set root delay
    response.data.rootDelay = htonl(rootDelay);
    // set root dispersion
    response.data.rootDispersion = htonl(rootDispersion);
    // set reference ID
    response.data.refID = refId;
    // set reference timestamp
    response.data.refTime = htonll(CLK_TAI_fromMono(lastUpdate) + NTP_UTC_OFFSET - clkTaiUtcOffset);
    // set origin and TX timestamps
    if (xleave < 0) {
        response.data.origTime = response.data.txTime;
        response.data.txTime = htonll(CLK_TAI() + NTP_UTC_OFFSET - clkTaiUtcOffset);
    }
    else {
        response.data.origTime = response.data.rxTime;
        response.data.txTime = tsXleave[xleave].txTime;
    }
    // set RX timestamp
    response.data.rxTime = htonll(rxTime);

    // finalize packet
    UDP_finalize(frame, flen);
    IPv4_finalize(frame, flen);
    // transmit packet
    NET_transmit(txDesc, flen);
}

// process peer response
static void ntpResponse(uint8_t *frame, const int flen) {
    // discard malformed packets
    if (flen < NTP4_SIZE)
        return;
    // map headers
    auto &response = PacketUDP<HEADER_NTP>::from(frame);

    // verify destination
    if (isMyMAC(response.eth.macDst))
        return;
    if (response.ip4.dst != ipAddress)
        return;
    // prevent loopback
    if (response.ip4.src == ipAddress)
        return;
    // filter non-server frames
    if (response.data.mode != NTP_MODE_SRV)
        return;
    // filter invalid frames
    if (response.data.origTime == 0)
        return;

    // locate recipient
    const uint32_t srcAddr = response.ip4.src;
    for (uint32_t i = 0; i < cntSources; i++) {
        const auto source = sources[i];

        // check for matching peer
        if (source == nullptr)
            return;
        if (!source->isRecipient(srcAddr))
            continue;

        // handoff frame to peer
        reinterpret_cast<ntp::Peer*>(source)->receive(frame, flen);
        return;
    }
}

static void runSelect(void *ref) {
    // prune defunct sources
    for (uint32_t i = 0; i < cntSources; i++) {
        if (sources[i]->isPruneable()) {
            deletePeer(sources[i]);
            break;
        }
    }

    // select best clock
    selectedSource = nullptr;
    for (uint32_t i = 0; i < cntSources; i++) {
        if (!sources[i]->isSelectable())
            continue;
        if (selectedSource == nullptr) {
            selectedSource = sources[i];
            continue;
        }
        if (sources[i]->getScore() < selectedSource->getScore()) {
            selectedSource = sources[i];
        }
    }

    // indicate complete loss of tracking
    if (selectedSource == nullptr) {
        refId = 0;
        clockStratum = 16;
        leapIndicator = 3;
        rootDelay = 0;
        rootDispersion = 0;
        return;
    }

    // sanity check source and check for update
    const auto source = selectedSource;
    source->select();
    if (lastUpdate == source->getLastUpdate())
        return;
    lastUpdate = source->getLastUpdate();

    // set status
    refId = source->getId();
    clockStratum = source->getStratum() + 1;
    leapIndicator = source->getLeapIndicator();
    rootDelay = source->getRootDelay() + static_cast<uint32_t>(0x1p16f * source->getDelayMean());
    rootDispersion = source->getRootDispersion() + static_cast<uint32_t>(0x1p16f * source->getDelayStdDev());

    // update offset compensation
    PLL_updateOffset(source->getPollingInterval(), source->getLastOffsetFixedPoint());
    // update frequency compensation
    PLL_updateDrift(source->getPollingInterval(), source->getFrequencyDrift());
}

static ntp::Source* newPeer(const uint32_t ipAddr) {
    const auto peerSlots = reinterpret_cast<ntp::Peer*>(rawPeers);
    for (int i = 0; i < MAX_NTP_PEERS; i++) {
        const auto slot = peerSlots + i;
        if (slot->isAllocated())
            continue;
        // append to source list
        sources[cntSources++] = new(slot) ntp::Peer(ipAddr);
        // return instance
        return slot;
    }
    return nullptr;
}

static void deletePeer(ntp::Source *peer) {
    // deselect source if selected
    if (peer == selectedSource)
        selectedSource = nullptr;

    // destroy  peer
    std::destroy_at(reinterpret_cast<ntp::Peer*>(peer));
    uint32_t slot = -1u;
    // locate matching slot
    for (uint32_t i = 0; i < cntSources; i++) {
        if (sources[i] == peer)
            slot = i;
    }
    if (slot > cntSources)
        return;
    // compact source list
    while (++slot < cntSources)
        sources[slot - 1] = sources[slot];
    // clear unused slots
    while (slot < MAX_NTP_SRCS)
        sources[slot++] = nullptr;
    // reduce source count
    --cntSources;
}

static void ntpDnsCallback(void *ref, const uint32_t addr) {
    // ignore if slots are full
    if (cntSources >= MAX_NTP_SRCS)
        return;
    // ignore own address
    if (addr == ipAddress)
        return;
    // verify address is not currently in use
    for (uint32_t i = 0; i < cntSources; i++) {
        if (sources[i]->isRecipient(addr))
            return;
    }
    // allocate new source
    newPeer(addr);
}

static void runDnsFill(void *ref) {
    if (cntSources < MAX_NTP_SRCS) {
        // fill with DHCP provided addresses
        uint32_t *addr;
        int cnt;
        DHCP_ntpAddr(&addr, &cnt);
        for (int i = 0; i < cnt; i++)
            ntpDnsCallback(nullptr, addr[i]);

        // fill with servers from public ntp pool
        DNS_lookup(NTP_POOL_FQDN, ntpDnsCallback, nullptr);
    }
}


// -----------------------------------------------------------------------------
// support for chronyc status queries ------------------------------------------
// -----------------------------------------------------------------------------

static constexpr int REQ_MIN_LEN = offsetof(CMD_Request, data.null.EOR);
static constexpr int REQ_MAX_LEN = sizeof(CMD_Request);

static constexpr int REP_LEN_NSOURCES = offsetof(CMD_Reply, data.n_sources.EOR);
static constexpr int REP_LEN_SOURCEDATA = offsetof(CMD_Reply, data.source_data.EOR);
static constexpr int REP_LEN_SOURCESTATS = offsetof(CMD_Reply, data.sourcestats.EOR);
static constexpr int REP_LEN_TRACKING = offsetof(CMD_Reply, data.tracking.EOR);
static constexpr int REP_LEN_NTP_DATA = offsetof(CMD_Reply, data.ntp_data.EOR);

// begin chronyc reply
static void chronycReply(CMD_Reply *cmdReply, const CMD_Request *cmdRequest);

// chronyc command handlers
static uint16_t chronycNSources(CMD_Reply *cmdReply, const CMD_Request *cmdRequest);
static uint16_t chronycSourceData(CMD_Reply *cmdReply, const CMD_Request *cmdRequest);
static uint16_t chronycSourceStats(CMD_Reply *cmdReply, const CMD_Request *cmdRequest);
static uint16_t chronycTracking(CMD_Reply *cmdReply, const CMD_Request *cmdRequest);
static uint16_t chronycNtpData(CMD_Reply *cmdReply, const CMD_Request *cmdRequest);

static const struct {
    uint16_t (*call)(CMD_Reply *cmdReply, const CMD_Request *cmdRequest);
    int len;
    uint16_t cmd;

    auto operator()(CMD_Reply *cmdReply, const CMD_Request *cmdRequest) const {
        return (*call)(cmdReply, cmdRequest);
    }
} handlers[] = {
    {chronycNSources, REP_LEN_NSOURCES, REQ_N_SOURCES},
    {chronycSourceData, REP_LEN_SOURCEDATA, REQ_SOURCE_DATA},
    {chronycSourceStats, REP_LEN_SOURCESTATS, REQ_SOURCESTATS},
    {chronycTracking, REP_LEN_TRACKING, REQ_TRACKING},
    {chronycNtpData, REP_LEN_NTP_DATA, REQ_NTP_DATA}
};

static void chronycRequest(uint8_t *frame, const int flen) {
    // drop invalid packets
    if (flen < UDP_DATA_OFFSET + REQ_MIN_LEN)
        return;
    if (flen > UDP_DATA_OFFSET + REQ_MAX_LEN)
        return;

    // map headers
    const auto &request = PacketUDP<CMD_Request>::from(frame);

    // drop invalid packets
    if (request.data.pkt_type != PKT_TYPE_CMD_REQUEST)
        return;
    if (request.data.pad1 != 0)
        return;
    if (request.data.pad2 != 0)
        return;

    // drop packet if nack is not possible
    if (request.data.version != PROTO_VERSION_NUMBER) {
        if (request.data.version < PROTO_VERSION_MISMATCH_COMPAT_SERVER)
            return;
    }

    const int txDesc = NET_getTxDesc();
    // allocate and clear frame buffer
    uint8_t *resp = NET_getTxBuff(txDesc);
    memset(resp, 0, UDP_DATA_OFFSET + sizeof(CMD_Reply));
    // return to sender
    memcpy(resp, frame, UDP_DATA_OFFSET);
    UDP_returnToSender(resp, ipAddress, DEFAULT_CANDM_PORT);

    // remap headers
    auto &response = PacketUDP<CMD_Reply>::from(resp);

    // begin response
    chronycReply(response.ptr(), request.ptr());

    // nack bad protocol version
    if (response.data.version != PROTO_VERSION_NUMBER) {
        response.data.status = htons(STT_BADPKTVERSION);
    }
    else {
        response.data.status = htons(STT_INVALID);
        const uint16_t cmd = htons(response.data.command);
        const int len = flen - UDP_DATA_OFFSET;
        for (const auto &handler : handlers) {
            if (handler.cmd == cmd) {
                if (handler.len == len)
                    response.data.status = handler(response.ptr(), request.ptr());
                else
                    response.data.status = htons(STT_BADPKTLENGTH);
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
    if (i >= cntSources)
        return htons(STT_NOSUCHSOURCE);
    // sanity check
    const auto source = sources[i];
    if (source == nullptr)
        return htons(STT_NOSUCHSOURCE);

    source->getSourceData(cmdReply->data.source_data);
    return htons(STT_SUCCESS);
}

static uint16_t chronycSourceStats(CMD_Reply *cmdReply, const CMD_Request *cmdRequest) {
    cmdReply->reply = htons(RPY_SOURCESTATS);

    const uint32_t i = htonl(cmdRequest->data.sourcestats.index);
    if (i >= cntSources)
        return htons(STT_NOSUCHSOURCE);
    // sanity check
    const ntp::Source *source = sources[i];
    if (source == nullptr)
        return htons(STT_NOSUCHSOURCE);

    source->getSourceStats(cmdReply->data.sourcestats);
    return htons(STT_SUCCESS);
}

static uint16_t chronycTracking(CMD_Reply *cmdReply, const CMD_Request *cmdRequest) {
    cmdReply->reply = htons(RPY_TRACKING);
    cmdReply->data.tracking.ref_id = refId;
    cmdReply->data.tracking.stratum = htons(clockStratum);
    cmdReply->data.tracking.leap_status = htons(leapIndicator);
    if (refId == NTP_REF_GPS) {
        cmdReply->data.tracking.ip_addr.family = htons(IPADDR_UNSPEC);
        cmdReply->data.tracking.ip_addr.addr.in4 = refId;
    }
    else {
        cmdReply->data.tracking.ip_addr.family = htons(IPADDR_INET4);
        cmdReply->data.tracking.ip_addr.addr.in4 = refId;
    }

    const ntp::Source *source = selectedSource;

    chrony::toTimespec(CLK_TAI_fromMono(lastUpdate) - clkTaiUtcOffset, &(cmdReply->data.tracking.ref_time));

    cmdReply->data.tracking.current_correction.f = chrony::htonf(PLL_offsetMean());
    cmdReply->data.tracking.last_offset.f = chrony::htonf(PLL_offsetLast());
    cmdReply->data.tracking.rms_offset.f = chrony::htonf(PLL_offsetRms());

    cmdReply->data.tracking.freq_ppm.f = chrony::htonf(-1e6f * (0x1p-32f * static_cast<float>(CLK_COMP_getComp())));
    cmdReply->data.tracking.resid_freq_ppm.f =
        chrony::htonf(-1e6f * (0x1p-32f * static_cast<float>(CLK_TAI_getTrim())));
    cmdReply->data.tracking.skew_ppm.f = chrony::htonf(1e6f * PLL_driftStdDev());

    cmdReply->data.tracking.root_delay.f = chrony::htonf(0x1p-16f * static_cast<float>(rootDelay));
    cmdReply->data.tracking.root_dispersion.f = chrony::htonf(0x1p-16f * static_cast<float>(rootDispersion));

    cmdReply->data.tracking.last_update_interval.f = chrony::htonf(
        source ? (0x1p-16f * static_cast<float>(1u << (16 + source->getPollingInterval()))) : 0);
    return htons(STT_SUCCESS);
}

static uint16_t chronycNtpData(CMD_Reply *cmdReply, const CMD_Request *cmdRequest) {
    cmdReply->reply = htons(RPY_NTP_DATA);

    // must be an IPv4 address
    if (cmdRequest->data.ntp_data.ip_addr.family != htons(IPADDR_INET4))
        return htons(STT_NOSUCHSOURCE);

    // locate server
    const uint32_t addr = cmdRequest->data.ntp_data.ip_addr.addr.in4;
    const ntp::Source *source = nullptr;
    for (uint32_t i = 0; i < cntSources; i++) {
        if (sources[i]->isRecipient(addr)) {
            source = sources[i];
            break;
        }
    }
    if (source == nullptr)
        return htons(STT_NOSUCHSOURCE);

    source->getNtpData(cmdReply->data.ntp_data);
    return htons(STT_SUCCESS);
}