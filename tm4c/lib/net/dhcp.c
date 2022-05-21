//
// Created by robert on 5/20/22.
//

#include <memory.h>
#include <stdint.h>

#include "../clk.h"
#include "eth.h"
#include "dhcp.h"
#include "ip.h"
#include "udp.h"
#include "util.h"
#include "../net.h"
#include "../../hw/sys.h"
#include "../led.h"


struct PACKED HEADER_DHCP {
    uint8_t OP;
    uint8_t HTYPE;
    uint8_t HLEN;
    uint8_t HOPS;
    uint8_t XID[4];
    uint8_t SECS[2];
    uint8_t FLAGS[2];
    uint8_t CIADDR[4];
    uint8_t YIADDR[4];
    uint8_t SIADDR[4];
    uint8_t GIADDR[4];
    uint8_t CHADDR[16];
    uint8_t SNAME[64];
    uint8_t FILE[128];
    uint8_t MAGIC[4];
};
_Static_assert(sizeof(struct HEADER_DHCP) == 240, "HEADER_DHCP must be 240 bytes");

static const uint8_t DHCP_MAGIC[] = { 0x63, 0x82, 0x53, 0x63 };
uint32_t dhcpXID = 0;
uint32_t dhcpLeaseExpire = 0;

static const uint8_t DHCP_OPT_DISCOVER[] = { 0x35, 0x01, 0x01 };
static const uint8_t DHCP_OPT_REQUEST[] = { 0x35, 0x01, 0x03 };
static const uint8_t DHCP_OPT_PARAM_REQ[] = { 0x37, 0x03, 0x01, 0x03, 0x06 };


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
    headerEth->ethType[0] = 0x08;
    headerEth->ethType[1] = 0x00;
    // IPv4 Header
    IPv4_init(frame);
    memset(headerIPv4->dst, 0xFF, 4);
    headerIPv4->proto = IP_PROTO_UDP;
    // UDP Header
    headerUDP->portSrc[1] = DHCP_PORT_CLI;
    headerUDP->portDst[1] = DHCP_PORT_SRV;
    // DHCP Header
    headerDHCP->OP = 1;
    headerDHCP->HTYPE = 1;
    headerDHCP->HLEN = 6;
    memcpy(headerDHCP->XID, &dhcpXID, sizeof(dhcpXID));
    getMAC(headerDHCP->CHADDR);
    memcpy(headerDHCP->MAGIC, DHCP_MAGIC, sizeof(DHCP_MAGIC));
}

void DHCP_poll() {
    const uint32_t now = CLK_MONOTONIC_INT();
    if(((int32_t)(dhcpLeaseExpire - now)) <= 0) {
        // re-attempt in 5 minutes if renewal fails
        dhcpLeaseExpire = now + 300;
        DHCP_renew();
    }
}

void DHCP_renew() {
    // compute new transaction ID
    dhcpXID = CLK_MONOTONIC_RAW();
    for(int i = 0; i < 4; i++)
        dhcpXID += UNIQUEID.WORD[i];

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
        // end mark
        frame[flen++] = 0xFF;
    } else {
        // renew existing lease
        copyIPv4(headerIPv4->src, &ipAddress);
        copyIPv4(headerDHCP->CIADDR, &ipAddress);
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

extern volatile uint8_t debugMac[6];
void DHCP_process(uint8_t *frame, int flen) {
    copyMAC(debugMac, frame);
    // discard malformed packets
    if(flen < 282) return;
    // map headers
    struct FRAME_ETH *headerEth = (struct FRAME_ETH *) frame;
    struct HEADER_IPv4 *headerIPv4 = (struct HEADER_IPv4 *) (headerEth + 1);
    struct HEADER_UDP *headerUDP = (struct HEADER_UDP *) (headerIPv4 + 1);
    struct HEADER_DHCP *headerDHCP = (struct HEADER_DHCP *) (headerUDP + 1);
    // discard if not from a server
    if(headerUDP->portSrc[0] != 0 || headerUDP->portSrc[1] != DHCP_PORT_SRV)
        return;
    // discard if not a server response
    if(headerDHCP->OP != 2) return;
    // discard if incorrect address type
    if(headerDHCP->HTYPE != 1) return;
    if(headerDHCP->HLEN != 6) return;
    // discard if MAGIC is incorrect
    if(memcmp(headerDHCP->MAGIC, DHCP_MAGIC, sizeof(DHCP_MAGIC)) != 0)
        return;
    // discard if XID is incorrect
    if(memcmp(headerDHCP->XID, &dhcpXID, sizeof(dhcpXID)) != 0)
        return;

    // relevant options
    uint32_t optSubnet = 0;
    uint32_t optRouter = 0;
    uint32_t optDNS = 0;
    uint32_t optDHCP = 0;
    uint32_t optLease = 0;
    uint8_t optMsgType = 0;

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
            copyIPv4(&optSubnet, ptr);
        // router address
        if(key == 3 && len >= 4)
            copyIPv4(&optRouter, ptr);
        // DNS server address
        if(key == 6 && len >= 4)
            copyIPv4(&optDNS, ptr);
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
            copyIPv4(&optDHCP, ptr);

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
        copyIPv4(&ipAddress, headerDHCP->YIADDR);
        ipSubnet = optSubnet;
        ipGateway = optRouter;
        ipDNS = optDNS;
        // set time of expiration (10% early)
        dhcpLeaseExpire = optLease;
        dhcpLeaseExpire -= optLease / 10;
        if(dhcpLeaseExpire > 86400) dhcpLeaseExpire = 86400;
        dhcpLeaseExpire += CLK_MONOTONIC_INT();
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
    copyIPv4(headerDHCP->CIADDR, response->YIADDR);
    copyIPv4(headerDHCP->SIADDR, response->SIADDR);
    // DHCPREQUEST
    memcpy(frame + flen, DHCP_OPT_REQUEST, sizeof(DHCP_OPT_REQUEST));
    flen += sizeof(DHCP_OPT_REQUEST);
    // parameter request
    memcpy(frame + flen, DHCP_OPT_PARAM_REQ, sizeof(DHCP_OPT_PARAM_REQ));
    flen += sizeof(DHCP_OPT_PARAM_REQ);
    // requested IP
    frame[flen++] = 50;
    frame[flen++] = 4;
    copyIPv4(frame + flen, response->YIADDR);
    flen += 4;
    // DHCP IP
    frame[flen++] = 54;
    frame[flen++] = 4;
    copyIPv4(frame + flen, response->SIADDR);
    flen += 4;
    // end mark
    frame[flen++] = 0xFF;
    // transmit frame
    UDP_finalize(frame, flen);
    IPv4_finalize(frame, flen);
    NET_transmit(txDesc, flen);
}
