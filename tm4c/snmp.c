//
// Created by robert on 5/23/22.
//

#include <memory.h>

#include "lib/format.h"
#include "lib/net.h"
#include "lib/net/eth.h"
#include "lib/net/ip.h"
#include "lib/net/udp.h"
#include "lib/net/util.h"
#include "lib/led.h"
#include "snmp.h"

#include "gitversion.h"

#define SNMP_PORT (161)

const uint8_t OID_PREFIX_MGMT[] = {0x2B, 6, 1, 2, 1 };

const uint8_t RESP_HEAD[] = { 0x02, 0x01, 0x00, 0x04, 0x06, 0x73, 0x74, 0x61, 0x74, 0x75, 0x73, 0xA2 };
const uint8_t RESP_ERR[] = { 0x02, 0x01, 0x00, 0x02, 0x01, 0x00, 0x30 };

static uint8_t buffOID[16];
static int lenOID;
static uint32_t reqId, errorStat, errorId;

int readLength(const uint8_t *data, int offset, int dlen, int *result);
int readBytes(const uint8_t *data, int offset, int dlen, void *result, int *rlen);
int readInt32(const uint8_t *data, int offset, int dlen, uint32_t *result);
int readOID(const uint8_t *data, int offset, int dlen, void *result, int *rlen);

int writeLength(uint8_t *data, int offset, int len);
int writeBytes(uint8_t *data, int offset, const void *value, int len);
int writeInt(uint8_t *data, int offset, void *value, int size);

int writeValueBytes(uint8_t *data, int offset, const uint8_t *prefOID, int prefLen, uint8_t oid, const void *value, int len);
int writeValueInt8(uint8_t *data, int offset, const uint8_t *prefOID, int prefLen, uint8_t oid, uint8_t value);
int writeValueInt16(uint8_t *data, int offset, const uint8_t *prefOID, int prefLen, uint8_t oid, uint16_t value);
int writeValueInt32(uint8_t *data, int offset, const uint8_t *prefOID, int prefLen, uint8_t oid, uint32_t value);
int writeValueInt64(uint8_t *data, int offset, const uint8_t *prefOID, int prefLen, uint8_t oid, uint64_t value);

int wrapVars(uint8_t *data, int offset, uint8_t *vars, int len);

void sendBatt(uint8_t *frame);
void sendGPSDO(uint8_t *frame);
void sendNTP(uint8_t *frame);

void SNMP_process(uint8_t *frame, int flen);

void SNMP_init() {
    UDP_register(SNMP_PORT, SNMP_process);
}

extern volatile uint8_t debugHex[32];

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
    int offset = 0, blen;
    uint8_t buff[16];
    // must be an ANS.1 sequence
    if(dataSNMP[offset++] != 0x30) return;
    // read sequence length
    int slen;
    offset = readLength(dataSNMP, offset, dlen, &slen);
    if(offset < 0) return;
    // validate message length
    if(slen != 0 && (dlen - offset) != slen) return;

    // SNMP version
    uint32_t version;
    offset = readInt32(dataSNMP, offset, dlen, &version);
    // only process version 1 packets
    if(offset < 0 || version != 0) return;

    // SNMP community
    blen = sizeof(buff);
    offset = readBytes(dataSNMP, offset, dlen, buff, &blen);
    // community must match "status"
    if(offset < 0 || blen != 6) return;
    if(memcmp(buff, "status", 6) != 0) return;

    // only accept get requests
    if(dataSNMP[offset++] != 0xA0) return;
    offset = readLength(dataSNMP, offset, dlen, &slen);
    if(slen != 0 && (dlen - offset) != slen) return;

    offset = readInt32(dataSNMP, offset, dlen, &reqId);
    offset = readInt32(dataSNMP, offset, dlen, &errorStat);
    offset = readInt32(dataSNMP, offset, dlen, &errorId);

    // request payload
    if(dataSNMP[offset++] != 0x30) return;
    offset = readLength(dataSNMP, offset, dlen, &slen);
    if(slen != 0 && (dlen - offset) != slen) return;

    // OID entry
    if(dataSNMP[offset++] != 0x30) return;
    offset = readLength(dataSNMP, offset, dlen, &slen);
    if(slen != 0 && (dlen - offset) != slen) return;

    // OID
    lenOID = sizeof(buffOID);
    offset = readOID(dataSNMP, offset, dlen, buffOID, &lenOID);
    if(lenOID >= sizeof(OID_PREFIX_MGMT)) {
        if(memcmp(buffOID, OID_PREFIX_MGMT, sizeof(OID_PREFIX_MGMT)) == 0) {
            uint16_t section = buffOID[sizeof(OID_PREFIX_MGMT)];
            if(section & 0x80) {
                section &= 0x7F;
                section <<= 7;
                section |= buffOID[sizeof(OID_PREFIX_MGMT)+1];
            }
            switch (section) {
                // GPSDO (Phyiscal Sensors)
                case 99:
                    sendGPSDO(frame);
                    break;
                // NTP
                case 197:
                    sendNTP(frame);
                    break;
                // battery
                case 233:
                    sendBatt(frame);
                    break;
                default:
                    break;
            }
        }
    }
}

int readLength(const uint8_t *data, int offset, int dlen, int *result) {
    if(offset < 0) return -1;
    if(offset >= dlen) return -1;
    uint8_t head = data[offset++];
    // definite short form
    if((head & 0x80) == 0) {
        *result = head;
        return offset;
    }
    // definite long form
    int len = head & 0x7F;
    if(len == 1) {
        if((offset + 1) >= dlen)
            return -1;
        *result = data[offset++];
        return offset;
    }
    if(len == 2) {
        if((offset + 2) >= dlen)
            return -1;
        uint16_t temp = data[offset++];
        temp <<= 8;
        temp |= data[offset++];
        *result = temp;
        return offset;
    }
    return -1;
}

int readBytes(const uint8_t *data, int offset, int dlen, void *result, int *rlen) {
    if(offset < 0) return -1;
    if(offset >= dlen) return -1;
    uint8_t tag = data[offset++];
    // verify type
    if(tag != 0x04) return -1;
    // get content length
    int blen;
    offset = readLength(data, offset, dlen, &blen);
    if(offset < 0 || (offset + blen) > dlen) return -1;
    if(rlen[0] > blen) rlen[0] = blen;
    for(int i = 0; i < rlen[0]; i++)
        ((uint8_t *) result)[i] = data[offset + i];
    return offset + blen;
}

int readInt32(const uint8_t *data, int offset, int dlen, uint32_t *result) {
    if(offset < 0) return -1;
    if(offset >= dlen) return -1;
    uint8_t tag = data[offset++];
    // verify type
    if(tag != 0x02) return -1;
    // get content length
    int blen;
    offset = readLength(data, offset, dlen, &blen);
    if(offset < 0 || (offset + blen) > dlen) return -1;
    uint32_t temp = 0;
    // odd byte sign extension
    if(blen & 1) {
        temp = (int32_t) ((int8_t) data[offset++]);
        blen -= 1;
    }
    // even byte sign extension
    else if(blen < 4) {
        temp = (int16_t ) __builtin_bswap16(*(uint16_t *) (data + offset));
        blen -= 2;
        offset += 2;
    }
    // append remaining 16-bit words
    for(int i = 0; i < blen; i += 2) {
        temp <<= 16;
        temp |= __builtin_bswap16(*(uint16_t *)(data + offset + i));
    }
    *result = temp;
    return offset + blen;
}

int readOID(const uint8_t *data, int offset, int dlen, void *result, int *rlen) {
    if(offset < 0) return -1;
    if(offset >= dlen) return -1;
    uint8_t tag = data[offset++];
    // verify type
    if(tag != 0x06) return -1;
    // get content length
    int blen;
    offset = readLength(data, offset, dlen, &blen);
    if(offset < 0 || (offset + blen) > dlen) return -1;
    if(rlen[0] > blen) rlen[0] = blen;
    for(int i = 0; i < rlen[0]; i++)
        ((uint8_t *) result)[i] = data[offset + i];
    return offset + blen;
}

int writeLength(uint8_t *data, int offset, int len) {
    if(len < 128) {
        data[offset++] = len;
        return offset;
    }
    if(len < (1<<8)) {
        data[offset++] = 0x81;
        data[offset++] = len;
        return offset;
    }
    if(len < (1<<16)) {
        data[offset++] = 0x82;
        data[offset++] = len >> 8;
        data[offset++] = len;
        return offset;
    }
    if(len < (1<<24)) {
        data[offset++] = 0x83;
        data[offset++] = len >> 16;
        data[offset++] = len >> 8;
        data[offset++] = len;
        return offset;
    }
    data[offset++] = 0x84;
    data[offset++] = len >> 24;
    data[offset++] = len >> 16;
    data[offset++] = len >> 8;
    data[offset++] = len;
    return offset;
}

int writeBytes(uint8_t *data, int offset, const void *value, int len) {
    data[offset++] = 0x04;
    data[offset++] = len;
    for(int i = 0; i < len; i++)
        data[offset++] = ((uint8_t *)value)[i];
    return offset;
}

int writeInt(uint8_t *data, int offset, void *value, int size) {
    data[offset++] = 0x02;
    data[offset++] = size--;
    while(size >= 0) {
        // append byte
        data[offset++] = ((uint8_t *) value)[size--];
    }
    return offset;
}

int writeValueBytes(
        uint8_t *data, int offset, const uint8_t *prefOID, int prefLen, uint8_t oid, const void *value, int len
) {
    uint8_t *buff = data + offset;
    len &= 0x7F;
    int vlen = 2;
    memcpy(buff + vlen, prefOID, prefLen);
    vlen += prefLen;
    buff[vlen++] = oid;
    vlen = writeBytes(buff, vlen, value, len);
    buff[0] = 0x30;
    buff[1] = vlen - 2;
    return offset + vlen;
}

int writeValueInt8(uint8_t *data, int offset, const uint8_t *prefOID, int prefLen, uint8_t oid, uint8_t value) {
    uint8_t *buff = data + offset;
    int vlen = 2;
    memcpy(buff + vlen, prefOID, prefLen);
    vlen += prefLen;
    buff[vlen++] = oid;
    vlen = writeInt(buff, vlen, &value, sizeof(value));
    buff[0] = 0x30;
    buff[1] = vlen - 2;
    return offset + vlen;
}

int writeValueInt16(uint8_t *data, int offset, const uint8_t *prefOID, int prefLen, uint8_t oid, uint16_t value) {
    uint8_t *buff = data + offset;
    int vlen = 2;
    memcpy(buff + vlen, prefOID, prefLen);
    vlen += prefLen;
    buff[vlen++] = oid;
    vlen = writeInt(buff, vlen, &value, sizeof(value));
    buff[0] = 0x30;
    buff[1] = vlen - 2;
    return offset + vlen;
}

int writeValueInt32(uint8_t *data, int offset, const uint8_t *prefOID, int prefLen, uint8_t oid, uint32_t value) {
    uint8_t *buff = data + offset;
    int vlen = 2;
    memcpy(buff + vlen, prefOID, prefLen);
    vlen += prefLen;
    buff[vlen++] = oid;
    vlen = writeInt(buff, vlen, &value, sizeof(value));
    buff[0] = 0x30;
    buff[1] = vlen - 2;
    return offset + vlen;
}

int writeValueInt64(uint8_t *data, int offset, const uint8_t *prefOID, int prefLen, uint8_t oid, uint64_t value) {
    uint8_t *buff = data + offset;
    int vlen = 2;
    memcpy(buff + vlen, prefOID, prefLen);
    vlen += prefLen;
    buff[vlen++] = oid;
    vlen = writeInt(buff, vlen, &value, sizeof(value));
    buff[0] = 0x30;
    buff[1] = vlen - 2;
    return offset + vlen;
}

int lengthSize(int len) {
    if(len < 128)
        return 1;
    if(len < (1<<8))
        return 2;
    if(len < (1<<16))
        return 3;
    if(len < (1<<24))
        return 4;
    return 5;
}

int wrapVars(uint8_t *data, int offset, uint8_t *vars, int len) {
    // request id
    uint8_t rid[8];
    int lenRID = writeInt(rid, 0, &reqId, sizeof(reqId));

    int tlen = len;
    tlen += lengthSize(len);
    tlen += sizeof(RESP_ERR);
    tlen += lenRID;
    int rlen = tlen;
    tlen += lengthSize(tlen);
    tlen += sizeof(RESP_HEAD);

    data[offset++] = 0x30;
    offset = writeLength(data, offset, tlen);

    memcpy(data + offset, RESP_HEAD, sizeof(RESP_HEAD));
    offset += sizeof(RESP_HEAD);

    offset = writeLength(data, offset, rlen);

    memcpy(data + offset, rid, lenRID);
    offset += lenRID;

    memcpy(data + offset, RESP_ERR, sizeof(RESP_ERR));
    offset += sizeof(RESP_ERR);

    offset = writeLength(data, offset, len);

    memcpy(data + offset, vars, len);
    offset += len;
    return offset;
}

void sendResults(uint8_t *frame, uint8_t *data, int dlen) {
    // map headers
    struct FRAME_ETH *headerEth = (struct FRAME_ETH *) frame;
    struct HEADER_IPv4 *headerIPv4 = (struct HEADER_IPv4 *) (headerEth + 1);
    struct HEADER_UDP *headerUDP = (struct HEADER_UDP *) (headerIPv4 + 1);

    // modify ethernet frame header
    copyMAC(headerEth->macDst, headerEth->macSrc);
    // modify IP header
    headerIPv4->dst = headerIPv4->src;
    headerIPv4->src = ipAddress;
    // modify UDP header
    headerUDP->portDst = headerUDP->portSrc;
    headerUDP->portSrc = __builtin_bswap16(SNMP_PORT);

    int txDesc = NET_getTxDesc();
    if(txDesc < 0) return;
    uint8_t *txFrame = NET_getTxBuff(txDesc);
    memcpy(txFrame, frame, UDP_DATA_OFFSET);

    dlen = wrapVars(txFrame, UDP_DATA_OFFSET, data, dlen);

    // transmit response
    UDP_finalize(txFrame, dlen);
    IPv4_finalize(txFrame, dlen);
    NET_transmit(txDesc, dlen);
}

// sub units
#include "snmp.batt.c"
#include "snmp.gpsdo.c"
#include "snmp.ntp.c"
