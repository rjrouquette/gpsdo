//
// Created by robert on 5/20/22.
//

#include "dhcp.hpp"

#include "arp.hpp"
#include "dns.hpp"
#include "eth.hpp"
#include "ip.hpp"
#include "udp.hpp"
#include "util.hpp"
#include "../net.h"
#include "../run.h"
#include "../../hw/sys.h"
#include "../clk/mono.h"

#include <memory.h>

#define DHCP_MAGIC (0x63538263)
#define REATTEMPT_INTVL (10)

struct [[gnu::packed]] HEADER_DHCP {
    uint8_t OP;
    uint8_t HTYPE;
    uint8_t HLEN;
    uint8_t HOPS;
    uint32_t XID;
    uint16_t SECS;
    uint16_t FLAGS;
    uint32_t CIADDR;
    uint32_t YIADDR;
    uint32_t SIADDR;
    uint32_t GIADDR;
    uint8_t CHADDR[16];
    uint8_t SNAME[64];
    uint8_t FILE[128];
    uint32_t MAGIC;
};

static_assert(sizeof(HEADER_DHCP) == 240, "HEADER_DHCP must be 240 bytes");

static uint32_t dhcpUUID;
static uint32_t dhcpXID = 0;
static uint32_t dhcpLeaseExpire = 0;

static const char lut_hex[] = "0123456789abcdef";
static char hostname[16] = {0};
static int lenHostname = 0;

static const uint8_t DHCP_OPT_DISCOVER[] = {53, 1, 1, 0};
static const uint8_t DHCP_OPT_REQUEST[] = {53, 1, 3, 0};
static const uint8_t DHCP_OPT_PARAM_REQ[] = {55, 4, 1, 3, 6, 42};

static uint32_t ntpAddr[8];
static int ntpLen;

static void processFrame(uint8_t *frame, int flen);
static void sendReply(HEADER_DHCP *response);

__attribute__((always_inline))
inline void pad_opts(uint8_t *frame, int *flen) {
    if ((*flen) & 1) {
        frame[(*flen)++] = 0;
    }
}

static void initPacket(void *frame) {
    const auto headerEth = static_cast<HEADER_ETH*>(frame);
    const auto headerIP4 = reinterpret_cast<HEADER_IP4*>(headerEth + 1);
    const auto headerUDP = reinterpret_cast<HEADER_UDP*>(headerIP4 + 1);
    const auto headerDHCP = reinterpret_cast<HEADER_DHCP*>(headerUDP + 1);

    // clear frame buffer
    memset(headerEth, 0, sizeof(HEADER_ETH));
    memset(headerIP4, 0, sizeof(HEADER_IP4));
    memset(headerUDP, 0, sizeof(HEADER_UDP));
    memset(headerDHCP, 0, sizeof(HEADER_DHCP));

    // broadcast MAC address
    broadcastMAC(headerEth->macDst);
    // IPv4 Header
    IPv4_init(static_cast<uint8_t*>(frame));
    headerIP4->dst = 0xFFFFFFFF;
    headerIP4->proto = IP_PROTO_UDP;
    // UDP Header
    headerUDP->portSrc = htons(DHCP_PORT_CLI);
    headerUDP->portDst = htons(DHCP_PORT_SRV);
    // DHCP Header
    headerDHCP->OP = 1;
    headerDHCP->HTYPE = 1;
    headerDHCP->HLEN = 6;
    headerDHCP->XID = dhcpXID;
    getMAC(headerDHCP->CHADDR);
    headerDHCP->MAGIC = DHCP_MAGIC;
}

static void dhcpRun(void *ref) {
    // link must be up to send packets
    if (!(NET_getPhyStatus() & PHY_STATUS_LINK))
        return;

    const uint32_t now = CLK_MONO_INT();
    if (static_cast<int32_t>(now - dhcpLeaseExpire) >= 0) {
        // re-attempt if renewal fails
        dhcpLeaseExpire += REATTEMPT_INTVL;
        DHCP_renew();
    }
}

void DHCP_init() {
    // compute UUID
    dhcpUUID = UNIQUEID.WORD[0];
    for (int i = 1; i < 4; i++)
        dhcpUUID += UNIQUEID.WORD[i];
    // create "unique" hostname
    strcpy(hostname, "gpsdo-");
    for (int i = 0; i < 4; i++) {
        hostname[6 + i] = lut_hex[(dhcpUUID >> (i * 4)) & 0xF];
    }
    hostname[10] = 0;
    lenHostname = 10;

    // register DHCP client port
    UDP_register(DHCP_PORT_CLI, processFrame);
    // wake every 0.5 seconds
    runSleep(RUN_SEC >> 1, dhcpRun, nullptr);
}

void DHCP_renew() {
    // create request ID
    dhcpXID = dhcpUUID + CLK_MONO_INT();

    // get TX descriptor
    const int txDesc = NET_getTxDesc();
    // initialize frame
    uint8_t *frame = NET_getTxBuff(txDesc);
    initPacket(frame);
    int flen = sizeof(HEADER_ETH);
    flen += sizeof(HEADER_IP4);
    flen += sizeof(HEADER_UDP);
    flen += sizeof(HEADER_DHCP);
    // map headers
    const auto headerEth = reinterpret_cast<HEADER_ETH*>(frame);
    const auto headerIP4 = reinterpret_cast<HEADER_IP4*>(headerEth + 1);
    const auto headerUDP = reinterpret_cast<HEADER_UDP*>(headerIP4 + 1);
    const auto headerDHCP = reinterpret_cast<HEADER_DHCP*>(headerUDP + 1);

    if (ipAddress == 0) {
        // acquire a new lease
        // DHCPDISCOVER
        memcpy(frame + flen, DHCP_OPT_DISCOVER, sizeof(DHCP_OPT_DISCOVER));
        flen += sizeof(DHCP_OPT_DISCOVER);
        // parameter request
        memcpy(frame + flen, DHCP_OPT_PARAM_REQ, sizeof(DHCP_OPT_PARAM_REQ));
        flen += sizeof(DHCP_OPT_PARAM_REQ);
        // hostname
        frame[flen++] = 12;
        frame[flen++] = lenHostname;
        memcpy(frame + flen, hostname, lenHostname);
        flen += lenHostname;
        pad_opts(frame, &flen);
        // end mark
        frame[flen++] = 0xFF;
        pad_opts(frame, &flen);
    }
    else {
        // renew existing lease
        headerIP4->src = ipAddress;
        headerDHCP->CIADDR = ipAddress;
        // DHCPREQUEST
        memcpy(frame + flen, DHCP_OPT_REQUEST, sizeof(DHCP_OPT_REQUEST));
        flen += sizeof(DHCP_OPT_REQUEST);
        // parameter request
        memcpy(frame + flen, DHCP_OPT_PARAM_REQ, sizeof(DHCP_OPT_PARAM_REQ));
        flen += sizeof(DHCP_OPT_PARAM_REQ);
        // end mark
        frame[flen++] = 0xFF;
        pad_opts(frame, &flen);
    }

    // transmit frame
    UDP_finalize(frame, flen);
    IPv4_finalize(frame, flen);
    NET_transmit(txDesc, flen);
}

uint32_t DHCP_expires() {
    return dhcpLeaseExpire - CLK_MONO_INT();
}

static void processFrame(uint8_t *frame, int flen) {
    // discard malformed packets
    if (flen < 282)
        return;
    // map headers
    const auto headerEth = reinterpret_cast<HEADER_ETH*>(frame);
    const auto headerIP4 = reinterpret_cast<HEADER_IP4*>(headerEth + 1);
    const auto headerUDP = reinterpret_cast<HEADER_UDP*>(headerIP4 + 1);
    const auto headerDHCP = reinterpret_cast<HEADER_DHCP*>(headerUDP + 1);
    // discard if not directly addressed to us
    if (isMyMAC(headerEth->macDst))
        return;
    // discard if not from a server
    if (headerUDP->portSrc != htons(DHCP_PORT_SRV))
        return;
    // discard if not a server response
    if (headerDHCP->OP != 2)
        return;
    // discard if incorrect address type
    if (headerDHCP->HTYPE != 1)
        return;
    if (headerDHCP->HLEN != 6)
        return;
    // discard if MAGIC is incorrect
    if (headerDHCP->MAGIC != DHCP_MAGIC)
        return;
    // discard if XID is incorrect
    if (headerDHCP->XID != dhcpXID)
        return;

    // relevant options
    uint32_t optSubnet = ipSubnet;
    uint32_t optDHCP = 0;
    uint32_t optLease = 0;
    uint8_t optMsgType = 0;
    const uint8_t *optDns = nullptr;
    int optDnsLen = 0;
    const uint8_t *optRouter = nullptr;
    int optRouterLen = 0;
    const uint8_t *optNtp = nullptr;
    int optNtpLen = 0;

    // parse options
    const uint8_t *ptr = (uint8_t*) (headerDHCP + 1);
    const uint8_t *end = frame + flen;
    while ((ptr + 1) < end) {
        // get field identifier
        const uint8_t key = ptr[0];

        // stop if end-mark
        if (key == 0xFF)
            break;

        // skip if padding
        if (key == 0x00) {
            ++ptr;
            continue;
        }

        // get field length
        const uint8_t len = ptr[1];
        // advance pointer
        ptr += 2;
        // stop if bad length
        if ((ptr + len) >= end)
            break;

        // subnet mask
        if (key == 1 && len == 4)
            memcpy(&optSubnet, ptr, 4);
        // router address
        if (key == 3 && (len & 3) == 0) {
            optRouter = ptr;
            optRouterLen = len;
        }
        // DNS server address
        if (key == 6 && (len & 3) == 0) {
            optDns = ptr;
            optDnsLen = len;
        }
        // NTP/SNTP server address
        if (key == 42 && (len & 3) == 0) {
            optNtp = ptr;
            optNtpLen = len;
        }
        // lease time
        if (key == 51 && len == 4) {
            const auto temp = reinterpret_cast<uint8_t*>(&optLease);
            temp[3] = ptr[0];
            temp[2] = ptr[1];
            temp[1] = ptr[2];
            temp[0] = ptr[3];
        }
        // message type
        if (key == 53)
            optMsgType = ptr[0];
        // dhcp server
        if (key == 54 && len == 4)
            memcpy(&optDHCP, ptr, 4);

        // advance pointer
        ptr += len;
    }

    // process DHCPOFFER
    if (optMsgType == 2) {
        sendReply(headerDHCP);
        return;
    }
    // process DHCPACK
    if (optMsgType == 5) {
        // set basic IPv4 address information
        ipAddress = headerDHCP->YIADDR;
        ipSubnet = optSubnet;
        ipBroadcast = ipAddress | (~ipSubnet);

        // set default gateway if available
        uint32_t addr;
        for (int i = 0; i < optRouterLen; i += 4) {
            memcpy(&addr, optRouter + i, sizeof(addr));
            // the router subnet must match our own
            if (!IPv4_testSubnet(ipSubnet, ipAddress, addr)) {
                ipRouter = addr;
                ARP_refreshRouter();
                break;
            }
        }

        // update DNS server
        for (int i = 0; i < optDnsLen; i += 4) {
            memcpy(&addr, optDns + i, sizeof(addr));
            // select first valid address
            if (addr) {
                ipDNS = addr;
                DNS_updateMAC();
                break;
            }
        }

        // update ntp server list
        if (optNtp != nullptr) {
            if (optNtpLen > static_cast<int>(sizeof(ntpAddr)))
                optNtpLen = sizeof(ntpAddr);
            memcpy(&ntpAddr, optNtp, optNtpLen);
            ntpLen = optNtpLen >> 2;
        }
        // set time of expiration (10% early)
        dhcpLeaseExpire = optLease;
        dhcpLeaseExpire -= optLease / 10;
        if (dhcpLeaseExpire > 86400)
            dhcpLeaseExpire = 86400;
        dhcpLeaseExpire += CLK_MONO_INT();
        return;
    }
}

static void sendReply(HEADER_DHCP *response) {
    // get TX descriptor
    const int txDesc = NET_getTxDesc();
    // initialize frame
    uint8_t *frame = NET_getTxBuff(txDesc);
    initPacket(frame);
    int flen = sizeof(HEADER_ETH);
    flen += sizeof(HEADER_IP4);
    flen += sizeof(HEADER_UDP);
    flen += sizeof(HEADER_DHCP);
    // map headers
    const auto headerEth = reinterpret_cast<HEADER_ETH*>(frame);
    const auto headerIP4 = reinterpret_cast<HEADER_IP4*>(headerEth + 1);
    const auto headerUDP = reinterpret_cast<HEADER_UDP*>(headerIP4 + 1);
    const auto headerDHCP = reinterpret_cast<HEADER_DHCP*>(headerUDP + 1);
    // request proposed lease
    headerDHCP->CIADDR = response->YIADDR;
    headerDHCP->SIADDR = response->SIADDR;
    // DHCPREQUEST
    memcpy(frame + flen, DHCP_OPT_REQUEST, sizeof(DHCP_OPT_REQUEST));
    flen += sizeof(DHCP_OPT_REQUEST);
    // parameter request
    memcpy(frame + flen, DHCP_OPT_PARAM_REQ, sizeof(DHCP_OPT_PARAM_REQ));
    flen += sizeof(DHCP_OPT_PARAM_REQ);
    // hostname (with zero padding
    frame[flen++] = 12;
    frame[flen++] = lenHostname;
    memcpy(frame + flen, hostname, lenHostname);
    flen += lenHostname;
    pad_opts(frame, &flen);
    // requested IP
    frame[flen++] = 50;
    frame[flen++] = 4;
    memcpy(frame + flen, &response->YIADDR, 4);
    flen += 4;
    // DHCP IP
    frame[flen++] = 54;
    frame[flen++] = 4;
    memcpy(frame + flen, &response->SIADDR, 4);
    flen += 4;
    // end mark
    frame[flen++] = 0xFF;
    pad_opts(frame, &flen);
    // transmit frame
    UDP_finalize(frame, flen);
    IPv4_finalize(frame, flen);
    NET_transmit(txDesc, flen);
}

const char* DHCP_hostname() {
    return hostname;
}

void DHCP_ntpAddr(uint32_t **addr, int *count) {
    *addr = ntpAddr;
    *count = ntpLen;
}
