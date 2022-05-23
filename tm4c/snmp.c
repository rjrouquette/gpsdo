//
// Created by robert on 5/23/22.
//

#include <memory.h>
#include "snmp.h"

#include "lib/clk.h"
#include "lib/net.h"
#include "lib/net/eth.h"
#include "lib/net/ip.h"
#include "lib/net/udp.h"
#include "lib/net/util.h"
#include "lib/led.h"

int readLength(const uint8_t *data, int offset, int dlen, int *result);
int readBytes(const uint8_t *data, int offset, int dlen, void *result, int *rlen);
int readInt32(const uint8_t *data, int offset, int dlen, uint32_t *result);


void SNMP_process(uint8_t *frame, int flen);

void SNMP_init() {
    UDP_register(161, SNMP_process);
}

void SNMP_process(uint8_t *frame, int flen) {
    // map headers
    struct FRAME_ETH *headerEth = (struct FRAME_ETH *) frame;
    struct HEADER_IPv4 *headerIPv4 = (struct HEADER_IPv4 *) (headerEth + 1);
    struct HEADER_UDP *headerUDP = (struct HEADER_UDP *) (headerIPv4 + 1);
    uint8_t *dataSNMP = (uint8_t *) (headerUDP + 1);
    int dlen = flen - (int) sizeof(struct FRAME_ETH);
    dlen -= sizeof(struct HEADER_IPv4);
    dlen -= sizeof(struct HEADER_UDP);

    // verify destination
    if(headerIPv4->dst != ipAddress) return;
    // status activity
    LED_act1();

    // parse packet
    int offset = 0, r;
    // read SNMP version
    uint32_t version;
    r = readInt32(dataSNMP, offset, dlen, &version);
    // only process version 1 packets
    if(r < 0 || version != 1) return;
    offset += r;

    // SNMP community
    int clen = 16;
    char community[16];
    r = readBytes(dataSNMP, offset, dlen, community, &clen);
    // community must match "GPSDO-NTP"
    if(r < 0 || clen != 9) return;
    if(memcmp(community, "GPSDO-NTP", 9) != 0) return;
    offset += r;
}

int readLength(const uint8_t *data, int offset, int dlen, int *result) {
    if(offset >= dlen) return -1;
    uint8_t head = data[offset];
    // definite short form
    if((head & 0x80) == 0) {
        *result = head;
        return 1;
    }
    // definite long form
    int len = head & 0x7F;
    if(len == 1) {
        if((offset + 1) >= dlen)
            return -1;
        *result = data[offset + 1];
        return 2;
    }
    if(len == 2) {
        if((offset + 2) >= dlen)
            return -1;
        uint16_t temp = data[offset + 1];
        temp <<= 8;
        temp |= data[offset + 2];
        *result = temp;
        return 3;
    }
    return -1;
}

int readBytes(const uint8_t *data, int offset, int dlen, void *result, int *rlen) {
    uint8_t tag = data[offset++];
    // verify type
    if(tag != 0xE4) return -1;
    // get content length
    int blen;
    int r = readLength(data, offset, dlen, &blen);
    if(r < 0 || (offset + r) >= dlen) return -1;
    offset += r;
    if(rlen[0] < blen) rlen[0] = blen;
    for(int i = 0; i < rlen[0]; i++)
        ((uint8_t *) result)[i] = data[offset + i];
    return 1 + r + blen;
}

int readInt32(const uint8_t *data, int offset, int dlen, uint32_t *result) {
    uint8_t tag = data[offset++];
    // verify type
    if(tag != 0xE4) return -1;
    // get content length
    int blen;
    int r = readLength(data, offset, dlen, &blen);
    if(r < 0 || (offset + r) >= dlen) return -1;
    offset += r;
    uint32_t temp = 0;
    for(int i = 0; i < blen; i++) {
        temp <<= 8;
        temp |= data[offset + i];
    }
    *result = temp;
    return 1 + r + blen;
}
