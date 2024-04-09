//
// Created by robert on 5/23/22.
//

#include "snmp.hpp"

#include "sensors.hpp"
#include "util.hpp"
#include "../led.h"
#include "../net.h"
#include "../net/eth.hpp"
#include "../net/ip.hpp"
#include "../net/udp.hpp"
#include "../net/util.hpp"

#include <memory.h>

namespace snmp {
    struct [[gnu::packed]] FrameSnmp : FrameUdp4 {
        static constexpr int DATA_OFFSET = FrameUdp4::DATA_OFFSET;

        uint8_t data[0];

        static auto& from(void *frame) {
            return *static_cast<FrameSnmp*>(frame);
        }

        static auto& from(const void *frame) {
            return *static_cast<const FrameSnmp*>(frame);
        }
    };

    static_assert(sizeof(FrameSnmp) == 42, "FrameSnmp must be 42 bytes");

    static constexpr int PORT = 161;

    static constexpr uint8_t OID_PREFIX_MGMT[] = {0x2B, 6, 1, 2, 1};
    static constexpr int OID_PREFIX_MGMT_LEN = sizeof(OID_PREFIX_MGMT);

    static uint8_t buffOID[16];
    static int lenOID;
    static uint32_t reqId, errorStat, errorId;

    static void sendBatt(const uint8_t *frame);
    static void sendSensors(const uint8_t *frame);

    static void process(uint8_t *frame, int flen);
}

void snmp::init() {
    UDP_register(PORT, process);
}

static void snmp::process(uint8_t *frame, int flen) {
    // map headers
    const auto &request = FrameSnmp::from(frame);
    const int dlen = flen - FrameSnmp::DATA_OFFSET;

    // verify destination
    if (isMyMAC(request.eth.macDst))
        return;
    if (request.ip4.dst != ipAddress)
        return;
    // status activity
    LED_act1();

    // parse packet
    const auto dataSNMP = request.data;
    int offset = 0;
    uint8_t buff[16];
    // must be an ANS.1 sequence
    if (dataSNMP[offset++] != 0x30)
        return;
    // read sequence length
    int slen;
    offset = readLength(dataSNMP, offset, dlen, slen);
    if (offset < 0)
        return;
    // validate message length
    if (slen != 0 && (dlen - offset) != slen)
        return;

    // SNMP version
    uint32_t version;
    offset = readInt32(dataSNMP, offset, dlen, version);
    // only process version 1 packets
    if (offset < 0 || version != 0)
        return;

    // SNMP community
    int blen = sizeof(buff);
    offset = readBytes(dataSNMP, offset, dlen, buff, blen);
    // community must match "status"
    if (offset < 0 || blen != 6)
        return;
    if (memcmp(buff, "status", 6) != 0)
        return;

    // only accept get requests
    if (dataSNMP[offset++] != 0xA0)
        return;
    offset = readLength(dataSNMP, offset, dlen, slen);
    if (slen != 0 && (dlen - offset) != slen)
        return;

    offset = readInt32(dataSNMP, offset, dlen, reqId);
    offset = readInt32(dataSNMP, offset, dlen, errorStat);
    offset = readInt32(dataSNMP, offset, dlen, errorId);

    // request payload
    if (dataSNMP[offset++] != 0x30)
        return;
    offset = readLength(dataSNMP, offset, dlen, slen);
    if (slen != 0 && (dlen - offset) != slen)
        return;

    // OID entry
    if (dataSNMP[offset++] != 0x30)
        return;
    offset = readLength(dataSNMP, offset, dlen, slen);
    if (slen != 0 && (dlen - offset) != slen)
        return;

    // OID
    lenOID = sizeof(buffOID);
    (void) readOID(dataSNMP, offset, dlen, buffOID, lenOID);
    if (lenOID >= OID_PREFIX_MGMT_LEN) {
        if (memcmp(buffOID, OID_PREFIX_MGMT, sizeof(OID_PREFIX_MGMT)) == 0) {
            uint16_t section = buffOID[sizeof(OID_PREFIX_MGMT)];
            if (section & 0x80) {
                section &= 0x7F;
                section <<= 7;
                section |= buffOID[sizeof(OID_PREFIX_MGMT) + 1];
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

static void sendResults(const uint8_t *frame, const uint8_t *data, int dlen) {
    // copy request frame
    const int txDesc = NET_getTxDesc();
    uint8_t *txFrame = NET_getTxBuff(txDesc);
    memcpy(txFrame, frame, UDP_DATA_OFFSET);

    // return frame to sender
    UDP_returnToSender(txFrame, ipAddress, snmp::PORT);

    // map headers
    auto &response = snmp::FrameSnmp::from(txFrame);

    int flen = snmp::FrameSnmp::DATA_OFFSET;
    flen += snmp::wrapVars(snmp::reqId, response.data, data, dlen);

    // transmit response
    UDP_finalize(txFrame, flen);
    IPv4_finalize(txFrame, flen);
    NET_transmit(txDesc, flen);
}

static void snmp::sendBatt(const uint8_t *frame) {
    sendResults(frame, nullptr, 0);
}

static void snmp::sendSensors(const uint8_t *frame) {
    // variable bindings
    uint8_t buffer[1024];
    int dlen;

    // filter MIB
    if (buffOID[sizeof(OID_PREFIX_MGMT) + 1] != 1)
        return;
    if (buffOID[sizeof(OID_PREFIX_MGMT) + 2] != 1)
        return;
    if (buffOID[sizeof(OID_PREFIX_MGMT) + 3] != 1)
        return;

    // select output data
    switch (buffOID[sizeof(OID_PREFIX_MGMT) + 4]) {
        case OID_SENSOR_TYPE:
            dlen = writeSensorTypes(buffer);
            sendResults(frame, buffer, dlen);
            break;
        case OID_SENSOR_SCALE:
            dlen = writeSensorScales(buffer);
            sendResults(frame, buffer, dlen);
            break;
        case OID_SENSOR_PREC:
            dlen = writeSensorPrecs(buffer);
            sendResults(frame, buffer, dlen);
            break;
        case OID_SENSOR_VALUE:
            dlen = writeSensorValues(buffer);
            sendResults(frame, buffer, dlen);
            break;
        case OID_SENSOR_STATUS:
            dlen = writeSensorStatuses(buffer);
            sendResults(frame, buffer, dlen);
            break;
        case OID_SENSOR_UNITS:
            dlen = writeSensorUnits(buffer);
            sendResults(frame, buffer, dlen);
            break;
        case OID_SENSOR_UPDATE_TIME:
            dlen = writeSensorUpdateTimes(buffer);
            sendResults(frame, buffer, dlen);
            break;
        case OID_SENSOR_UPDATE_RATE:
            dlen = writeSensorUpdateRates(buffer);
            sendResults(frame, buffer, dlen);
            break;
        case OID_SENSOR_NAME:
            dlen = writeSensorNames(buffer);
            sendResults(frame, buffer, dlen);
            break;
        default:
            sendResults(frame, nullptr, 0);
            break;
    }
}
