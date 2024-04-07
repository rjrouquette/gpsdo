//
// Created by robert on 5/23/22.
//

#include <memory.h>

#include "../led.h"
#include "../net.h"
#include "../net/eth.h"
#include "../net/ip.h"
#include "../net/udp.h"
#include "../net/util.h"
#include "sensors.h"
#include "util.h"
#include "snmp.h"

#include "gitversion.h"

#define SNMP_PORT (161)

const uint8_t OID_PREFIX_MGMT[] = {0x2B, 6, 1, 2, 1 };

static uint8_t buffOID[16];
static int lenOID;
static uint32_t reqId, errorStat, errorId;

int readLength(const uint8_t *data, int offset, int dlen, int *result);
int readBytes(const uint8_t *data, int offset, int dlen, void *result, int *rlen);
int readInt32(const uint8_t *data, int offset, int dlen, uint32_t *result);
int readOID(const uint8_t *data, int offset, int dlen, void *result, int *rlen);

void sendBatt(uint8_t *frame);
void sendSensors(uint8_t *frame);

void SNMP_process(uint8_t *frame, int flen);

void SNMP_init() {
    UDP_register(SNMP_PORT, SNMP_process);
}

void SNMP_process(uint8_t *frame, int flen) {
    // map headers
    HEADER_ETH *headerEth = (HEADER_ETH *) frame;
    HEADER_IP4 *headerIP4 = (HEADER_IP4 *) (headerEth + 1);
    HEADER_UDP *headerUDP = (HEADER_UDP *) (headerIP4 + 1);
    uint8_t *dataSNMP = (uint8_t *) (headerUDP + 1);
    int dlen = flen - (int) sizeof(HEADER_ETH);
    dlen -= sizeof(HEADER_IP4);
    dlen -= sizeof(HEADER_UDP);

    // verify destination
    if(isMyMAC(headerEth->macDst)) return;
    if(headerIP4->dst != ipAddress) return;
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
                // GPSDO (Physical Sensors)
                case 99:
                    sendSensors(frame);
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

void sendResults(uint8_t *frame, uint8_t *data, int dlen) {
    // map headers
    HEADER_ETH *headerEth = (HEADER_ETH *) frame;
    HEADER_IP4 *headerIP4 = (HEADER_IP4 *) (headerEth + 1);
    HEADER_UDP *headerUDP = (HEADER_UDP *) (headerIP4 + 1);

    // modify ethernet frame header
    copyMAC(headerEth->macDst, headerEth->macSrc);
    // modify IP header
    headerIP4->dst = headerIP4->src;
    headerIP4->src = ipAddress;
    // modify UDP header
    headerUDP->portDst = headerUDP->portSrc;
    headerUDP->portSrc = __builtin_bswap16(SNMP_PORT);

    const int txDesc = NET_getTxDesc();
    uint8_t *txFrame = NET_getTxBuff(txDesc);
    memcpy(txFrame, frame, UDP_DATA_OFFSET);

    int flen = UDP_DATA_OFFSET;
    flen += SNMP_wrapVars(reqId, txFrame + flen, data, dlen);

    // transmit response
    UDP_finalize(txFrame, flen);
    IPv4_finalize(txFrame, flen);
    NET_transmit(txDesc, flen);
}

// sub units
#include "snmp.batt.c"

void sendSensors(uint8_t *frame) {
    // variable bindings
    uint8_t buffer[1024];
    int dlen;

    // filter MIB
    if(buffOID[sizeof(OID_PREFIX_MGMT)+1] != 1) return;
    if(buffOID[sizeof(OID_PREFIX_MGMT)+2] != 1) return;
    if(buffOID[sizeof(OID_PREFIX_MGMT)+3] != 1) return;

    // select output data
    switch (buffOID[sizeof(OID_PREFIX_MGMT)+4]) {
        case OID_SENSOR_TYPE:
            dlen = SNMP_writeSensorTypes(buffer);
            sendResults(frame, buffer, dlen);
            break;
        case OID_SENSOR_SCALE:
            dlen = SNMP_writeSensorScales(buffer);
            sendResults(frame, buffer, dlen);
            break;
        case OID_SENSOR_PREC:
            dlen = SNMP_writeSensorPrecs(buffer);
            sendResults(frame, buffer, dlen);
            break;
        case OID_SENSOR_VALUE:
            dlen = SNMP_writeSensorValues(buffer);
            sendResults(frame, buffer, dlen);
            break;
        case OID_SENSOR_STATUS:
            dlen = SNMP_writeSensorStatuses(buffer);
            sendResults(frame, buffer, dlen);
            break;
        case OID_SENSOR_UNITS:
            dlen = SNMP_writeSensorUnits(buffer);
            sendResults(frame, buffer, dlen);
            break;
        case OID_SENSOR_UPDATE_TIME:
            dlen = SNMP_writeSensorUpdateTimes(buffer);
            sendResults(frame, buffer, dlen);
            break;
        case OID_SENSOR_UPDATE_RATE:
            dlen = SNMP_writeSensorUpdateRates(buffer);
            sendResults(frame, buffer, dlen);
            break;
        case OID_SENSOR_NAME:
            dlen = SNMP_writeSensorNames(buffer);
            sendResults(frame, buffer, dlen);
            break;
    }
}
