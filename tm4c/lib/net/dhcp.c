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
    uint8_t BOOTP[192];
    uint8_t MAGIC[4];
};
_Static_assert(sizeof(struct HEADER_DHCP) == 240, "HEADER_DHCP must be 240 bytes");

static const uint32_t DHCP_MAGIC = 0x63825363;
static uint8_t dhcpXID[4];
static uint32_t dhcpLeaseExpire = 0;

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
    memcpy(headerDHCP->XID, dhcpXID, sizeof(dhcpXID));
    getMAC(headerDHCP->CHADDR);
    memcpy(headerDHCP->MAGIC, &DHCP_MAGIC, sizeof(DHCP_MAGIC));
}

void DHCP_poll() {
    uint32_t now = CLK_MONOTONIC_INT();
    if((dhcpLeaseExpire - now) < 0) {
        // re-attempt in 5 minutes if renewal fails
        dhcpLeaseExpire = now + 300;
        DHCP_renew();
    }
}

void DHCP_renew() {

}

void DHCP_release() {

}

void DHCP_process(uint8_t *frame, int len) {
    // discard malformed packets
    if(len < 282) return;
    // map headers
    struct FRAME_ETH *headerEth = (struct FRAME_ETH *) frame;
    struct HEADER_IPv4 *headerIPv4 = (struct HEADER_IPv4 *) (headerEth + 1);
    struct HEADER_UDP *headerUDP = (struct HEADER_UDP *) (headerIPv4 + 1);
    struct HEADER_DHCP *headerDHCP = (struct HEADER_DHCP *) (headerUDP + 1);
    // discard if not from a server
    if(headerUDP->portSrc[0] != 0 || headerUDP->portSrc[1] != DHCP_PORT_SRV)
        return;
    // discard if not a server response
    if(headerDHCP->OP != 2)
        return;
    // discard if MAGIC is incorrect
    if(memcmp(headerDHCP->MAGIC, &DHCP_MAGIC, sizeof(DHCP_MAGIC)) != 0)
        return;
    // discard if XID is incorrect
    if(memcmp(headerDHCP->XID, dhcpXID, sizeof(dhcpXID)) != 0)
        return;
}
