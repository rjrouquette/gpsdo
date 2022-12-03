//
// Created by robert on 12/2/22.
//

#include <string.h>

#include "status.h"
#include "lib/clk.h"
#include "lib/format.h"
#include "lib/led.h"
#include "lib/net.h"
#include "lib/net/eth.h"
#include "lib/net/ip.h"
#include "lib/net/udp.h"
#include "lib/net/util.h"
#include "gpsdo.h"
#include "hw/emac.h"
#include "lib/net/dhcp.h"

#define STATUS_PORT (23) // telnet port

void STATUS_process(uint8_t *frame, int flen);

unsigned statusEth(char *body);
unsigned statusGPS(char *body);
unsigned statusGPSDO(char *body);
unsigned statusNTP(char *body);

void STATUS_init() {
    UDP_register(STATUS_PORT, STATUS_process);
}

void STATUS_process(uint8_t *frame, int flen) {
    // restrict length
    if(flen > 128) return;
    // map headers
    struct FRAME_ETH *headerEth = (struct FRAME_ETH *) frame;
    struct HEADER_IPv4 *headerIPv4 = (struct HEADER_IPv4 *) (headerEth + 1);
    struct HEADER_UDP *headerUDP = (struct HEADER_UDP *) (headerIPv4 + 1);
    // verify destination
    if(headerIPv4->dst != ipAddress) return;
    // restrict length
    unsigned size = __builtin_bswap16(headerUDP->length) - sizeof(struct HEADER_UDP);
    if(size > 63) return;
    // status activity
    LED_act1();

    // modify ethernet frame header
    copyMAC(headerEth->macDst, headerEth->macSrc);
    // modify IP header
    headerIPv4->dst = headerIPv4->src;
    headerIPv4->src = ipAddress;
    // modify UDP header
    headerUDP->portDst = headerUDP->portSrc;
    headerUDP->portSrc = __builtin_bswap16(STATUS_PORT);

    // get packet body
    char *body = (char *) (headerUDP + 1);
    // force null termination
    body[size] = 0;
    if(strcmp(body, "ethernet") == 0) {
        size = statusEth(body);
    } else {
        char tmp[64];
        strcpy(tmp, body);
        strcpy(body, "invalid command: ");
        strcpy(body + 17, tmp);
        size = strlen(body);
    }

    // finalize response
    flen = UDP_DATA_OFFSET + size;
    UDP_finalize(frame, flen);
    IPv4_finalize(frame, flen);
    // transmit response
    int txDesc = NET_getTxDesc();
    if(txDesc < 0) return;
    memcpy(NET_getTxBuff(txDesc), frame, flen);
    NET_transmit(txDesc, flen);
}

unsigned statusEth(char *body) {
    char tmp[32];
    char *end = body;

    // dhcp hostname
    end = append(end, "hostname: ");
    end = append(end, DHCP_hostname());
    end = append(end, "\n");

    // mac address
    NET_getMacAddress(tmp);
    end = append(end, "mac address: ");
    end = append(end, tmp);
    end = append(end, "\n");

    // ip address
    NET_getIpAddress(tmp);
    end = append(end, "ip address: ");
    end = append(end, tmp);
    end = append(end, "\n");

    return end - body;
}

unsigned statusGPS(char *body) {
    return 0;
}

unsigned statusGPSDO(char *body) {
    return 0;
}

unsigned statusNTP(char *body) {
    return 0;
}
