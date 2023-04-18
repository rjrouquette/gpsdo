//
// Created by robert on 5/20/22.
//

#include <memory.h>
#include <stdint.h>

#include "../../hw/sys.h"
#include "../../ntp.h"
#include "../clk.h"
#include "../net.h"
#include "eth.h"
#include "dhcp.h"
#include "ip.h"
#include "udp.h"
#include "util.h"


struct PACKED HEADER_DHCP {
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
_Static_assert(sizeof(struct HEADER_DHCP) == 240, "HEADER_DHCP must be 240 bytes");

#define DHCP_MAGIC (0x63538263)
static uint32_t dhcpUUID;
static uint32_t dhcpXID = 0;
static uint32_t dhcpLeaseExpire = 0;

static const char lut_hex[] = "0123456789abcdef";
static char hostname[16] = { 0 };
static int lenHostname = 0;

static const uint8_t DHCP_OPT_DISCOVER[] = { 0x35, 0x01, 0x01 };
static const uint8_t DHCP_OPT_REQUEST[] = { 0x35, 0x01, 0x03 };
static const uint8_t DHCP_OPT_PARAM_REQ[] = { 0x37, 0x04, 0x01, 0x03, 0x06, 0x2A };

static void processFrame(uint8_t *frame, int flen);
static void sendReply(struct HEADER_DHCP *response);

static void initPacket(void *frame) {
    struct FRAME_ETH *headerEth = (struct FRAME_ETH *) frame;
    struct HEADER_IPv4 *headerIPv4 = (struct HEADER_IPv4 *) (headerEth + 1);
    struct HEADER_UDP *headerUDP = (struct HEADER_UDP *) (headerIPv4 + 1);
    struct HEADER_DHCP *headerDHCP = (struct HEADER_DHCP *) (headerUDP + 1);

    // clear frame buffer
    memset(headerEth, 0, sizeof(struct FRAME_ETH));
    memset(headerIPv4, 0, sizeof(struct HEADER_IPv4));
    memset(headerUDP, 0, sizeof(struct HEADER_UDP));
    memset(headerDHCP, 0, sizeof(struct HEADER_DHCP));

    // broadcast MAC address
    broadcastMAC(headerEth->macDst);
    // EtherType = IPv4
    headerEth->ethType = ETHTYPE_IPv4;
    // IPv4 Header
    IPv4_init(frame);
    headerIPv4->dst = 0xFFFFFFFF;
    headerIPv4->proto = IP_PROTO_UDP;
    // UDP Header
    headerUDP->portSrc = __builtin_bswap16(DHCP_PORT_CLI);
    headerUDP->portDst = __builtin_bswap16(DHCP_PORT_SRV);
    // DHCP Header
    headerDHCP->OP = 1;
    headerDHCP->HTYPE = 1;
    headerDHCP->HLEN = 6;
    headerDHCP->XID = dhcpXID;
    getMAC(headerDHCP->CHADDR);
    headerDHCP->MAGIC = DHCP_MAGIC;
}

void DHCP_init() {
    // compute UUID
    dhcpUUID = UNIQUEID.WORD[0];
    for(int i = 1; i < 4; i++)
        dhcpUUID += UNIQUEID.WORD[i];
    // create "unique" hostname
    strcpy(hostname, "gpsdo-");
    for(int i = 0; i < 4; i++) {
        hostname[6 + i] = lut_hex[(dhcpUUID >> (i * 4)) & 0xF];
    }
    hostname[10] = 0;
    lenHostname = 10;

    // register DHCP client port
    UDP_register(DHCP_PORT_CLI, processFrame);
}

void DHCP_run() {
    const uint32_t now = CLK_MONOTONIC_INT();
    if(((int32_t)(dhcpLeaseExpire - now)) <= 0) {
        // re-attempt in 10 seconds if renewal fails
        dhcpLeaseExpire = now + 10;
        DHCP_renew();
    }
}

void DHCP_renew() {
    // create request ID
    dhcpXID = dhcpUUID + CLK_MONOTONIC_INT();

    // get TX descriptor
    int txDesc = NET_getTxDesc();
    if(txDesc < 0) return;

    // initialize frame
    uint8_t *frame = NET_getTxBuff(txDesc);
    initPacket(frame);
    int flen = sizeof(struct FRAME_ETH);
    flen += sizeof(struct HEADER_IPv4);
    flen += sizeof(struct HEADER_UDP);
    flen += sizeof(struct HEADER_DHCP);
    // map headers
    struct FRAME_ETH *headerEth = (struct FRAME_ETH *) frame;
    struct HEADER_IPv4 *headerIPv4 = (struct HEADER_IPv4 *) (headerEth + 1);
    struct HEADER_UDP *headerUDP = (struct HEADER_UDP *) (headerIPv4 + 1);
    struct HEADER_DHCP *headerDHCP = (struct HEADER_DHCP *) (headerUDP + 1);

    if(ipAddress == 0) {
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
        memcpy(frame+flen, hostname, lenHostname);
        flen += lenHostname;
        // end mark
        frame[flen++] = 0xFF;
    } else {
        // renew existing lease
        headerIPv4->src = ipAddress;
        headerDHCP->CIADDR = ipAddress;
        // DHCPREQUEST
        memcpy(frame + flen, DHCP_OPT_REQUEST, sizeof(DHCP_OPT_REQUEST));
        flen += sizeof(DHCP_OPT_REQUEST);
        // end mark
        frame[flen++] = 0xFF;
    }

    // transmit frame
    UDP_finalize(frame, flen);
    IPv4_finalize(frame, flen);
    NET_transmit(txDesc, flen);
}

static void processFrame(uint8_t *frame, int flen) {
    // discard malformed packets
    if(flen < 282) return;
    // map headers
    struct FRAME_ETH *headerEth = (struct FRAME_ETH *) frame;
    struct HEADER_IPv4 *headerIPv4 = (struct HEADER_IPv4 *) (headerEth + 1);
    struct HEADER_UDP *headerUDP = (struct HEADER_UDP *) (headerIPv4 + 1);
    struct HEADER_DHCP *headerDHCP = (struct HEADER_DHCP *) (headerUDP + 1);
    // discard if not from a server
    if(headerUDP->portSrc != __builtin_bswap16(DHCP_PORT_SRV))
        return;
    // discard if not a server response
    if(headerDHCP->OP != 2) return;
    // discard if incorrect address type
    if(headerDHCP->HTYPE != 1) return;
    if(headerDHCP->HLEN != 6) return;
    // discard if MAGIC is incorrect
    if(headerDHCP->MAGIC != DHCP_MAGIC)
        return;
    // discard if XID is incorrect
    if(headerDHCP->XID != dhcpXID)
        return;

    // relevant options
    uint32_t optSubnet = ipSubnet;
    uint32_t optRouter = ipGateway;
    uint32_t optDNS = ipDNS;
    uint32_t optDHCP = 0;
    uint32_t optLease = 0;
    uint8_t optMsgType = 0;
    uint8_t *optNTP = 0;
    int lenNTP = 0;

    // parse options
    uint8_t *ptr = (uint8_t *) (headerDHCP + 1);
    uint8_t *end = frame + flen - 4;
    while((ptr + 1) < end) {
        // get field identifier
        const uint8_t key = ptr[0];

        // stop if end-mark
        if(key == 0xFF) break;

        // skip if padding
        if(key == 0x00)  {
            ++ptr;
            continue;
        }

        // get field length
        const uint8_t len = ptr[1];
        // advance pointer
        ptr += 2;
        // stop if bad length
        if((ptr + len) >= end) break;

        // subnet mask
        if(key == 1 && len == 4)
            memcpy(&optSubnet, ptr, 4);
        // router address
        if(key == 3 && len >= 4)
            memcpy(&optRouter, ptr, 4);
        // DNS server address
        if(key == 6 && len >= 4)
            memcpy(&optDNS, ptr, 4);
        // NTP/SNTP server address
        if(key == 42 && len >= 4) {
            optNTP = ptr;
            lenNTP = len;
        }
        // message type
        if(key == 53)
            optMsgType = ptr[0];
        // lease time
        if(key == 51 && len == 4) {
            uint8_t *temp = (uint8_t *) &optLease;
            temp[3] = ptr[0];
            temp[2] = ptr[1];
            temp[1] = ptr[2];
            temp[0] = ptr[3];
        }
        // dhcp server
        if(key == 54 && len == 4)
            memcpy(&optDHCP, ptr, 4);

        // advance pointer
        ptr += len;
    }

    // process DHCPOFFER
    if(optMsgType == 2) {
        sendReply(headerDHCP);
        return;
    }
    // process DHCPACK
    if(optMsgType == 5) {
        ipAddress = headerDHCP->YIADDR;
        ipSubnet = optSubnet;
        ipBroadcast = (ipAddress & ipSubnet) | (~ipSubnet);
        ipGateway = optRouter;
        ipDNS = optDNS;
        // set time of expiration (10% early)
        dhcpLeaseExpire = optLease;
        dhcpLeaseExpire -= optLease / 10;
        if(dhcpLeaseExpire > 86400) dhcpLeaseExpire = 86400;
        dhcpLeaseExpire += CLK_MONOTONIC_INT();
        // set NTP servers
        if(lenNTP > 0 && ((lenNTP & 0x3) == 0)) {
            int i = 0;
            for(int j = 0; j < lenNTP; j += 4) {
                uint32_t addr = *(uint32_t *)(optNTP + j);
                if(addr != ipAddress)
                    NTP_setServer(i++, addr);
            }
        }
        return;
    }
}

static void sendReply(struct HEADER_DHCP *response) {
    // get TX descriptor
    int txDesc = NET_getTxDesc();
    if(txDesc < 0) return;
    // initialize frame
    uint8_t *frame = NET_getTxBuff(txDesc);
    initPacket(frame);
    int flen = sizeof(struct FRAME_ETH);
    flen += sizeof(struct HEADER_IPv4);
    flen += sizeof(struct HEADER_UDP);
    flen += sizeof(struct HEADER_DHCP);
    // map headers
    struct FRAME_ETH *headerEth = (struct FRAME_ETH *) frame;
    struct HEADER_IPv4 *headerIPv4 = (struct HEADER_IPv4 *) (headerEth + 1);
    struct HEADER_UDP *headerUDP = (struct HEADER_UDP *) (headerIPv4 + 1);
    struct HEADER_DHCP *headerDHCP = (struct HEADER_DHCP *) (headerUDP + 1);
    // request proposed lease
    headerDHCP->CIADDR = response->YIADDR;
    headerDHCP->SIADDR = response->SIADDR;
    // DHCPREQUEST
    memcpy(frame + flen, DHCP_OPT_REQUEST, sizeof(DHCP_OPT_REQUEST));
    flen += sizeof(DHCP_OPT_REQUEST);
    // parameter request
    memcpy(frame + flen, DHCP_OPT_PARAM_REQ, sizeof(DHCP_OPT_PARAM_REQ));
    flen += sizeof(DHCP_OPT_PARAM_REQ);
    // hostname
    frame[flen++] = 12;
    frame[flen++] = lenHostname;
    memcpy(frame+flen, hostname, lenHostname);
    flen += lenHostname;
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
    // transmit frame
    UDP_finalize(frame, flen);
    IPv4_finalize(frame, flen);
    NET_transmit(txDesc, flen);
}

const char * DHCP_hostname() {
    return hostname;
}
