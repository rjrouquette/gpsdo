//
// Created by robert on 5/26/22.
//

#include <math.h>
#include "lib/temp.h"
#include "gpsdo.h"

const uint8_t OID_SENSOR_PREFIX[] = { 0x06, 0x0A, 0x2B, 6, 1, 2, 1, 99, 1, 1, 1 };

#define OID_SENSOR_TYPE (1)
#define OID_SENSOR_SCALE (2)
#define OID_SENSOR_PREC (3)
#define OID_SENSOR_VALUE (4)

#define OID_SENSOR_UNITS (6)

#define OID_SENSOR_TYPE_OTHER (1)
#define OID_SENSOR_TYPE_UNKNOWN (2)
#define OID_SENSOR_TYPE_VOLTS_AC (3)
#define OID_SENSOR_TYPE_VOLTS_DC (4)
#define OID_SENSOR_TYPE_AMPS (5)
#define OID_SENSOR_TYPE_WATTS (6)
#define OID_SENSOR_TYPE_HERTZ (7)
#define OID_SENSOR_TYPE_CELSIUS (8)
#define OID_SENSOR_TYPE_PERC_REL_HUM (9)
#define OID_SENSOR_TYPE_RPM (10)
#define OID_SENSOR_TYPE_CUB_MTR_MIN (11)
#define OID_SENSOR_TYPE_BOOL (12)

#define OID_SENSOR_SCALE_1E_24 (1)
#define OID_SENSOR_SCALE_1E_21 (2)
#define OID_SENSOR_SCALE_1E_18 (3)
#define OID_SENSOR_SCALE_1E_15 (4)
#define OID_SENSOR_SCALE_1E_12 (5)
#define OID_SENSOR_SCALE_1E_9 (6)
#define OID_SENSOR_SCALE_1E_6 (7)
#define OID_SENSOR_SCALE_1E_3 (8)
#define OID_SENSOR_SCALE_1 (9)
#define OID_SENSOR_SCALE_1E3 (10)
#define OID_SENSOR_SCALE_1E6 (11)
#define OID_SENSOR_SCALE_1E9 (12)
#define OID_SENSOR_SCALE_1E12 (13)
#define OID_SENSOR_SCALE_1E15 (14)
#define OID_SENSOR_SCALE_1E18 (15)
#define OID_SENSOR_SCALE_1E21 (16)
#define OID_SENSOR_SCALE_1E24 (17)

int writeGpsdoType(uint8_t *buffer);
int writeGpsdoScale(uint8_t *buffer);
int writeGpsdoPrec(uint8_t *buffer);
int writeGpsdoValue(uint8_t *buffer);
int writeGpsdoUnits(uint8_t *buffer);

void sendGPSDO(uint8_t *frame) {
    // variable bindings
    uint8_t buffer[512];
    int dlen;

    // filter MIB
    if(buffOID[sizeof(OID_PREFIX_MGMT)+1] != 1) return;
    if(buffOID[sizeof(OID_PREFIX_MGMT)+2] != 1) return;
    if(buffOID[sizeof(OID_PREFIX_MGMT)+3] != 1) return;

    // select output data
    switch (buffOID[sizeof(OID_PREFIX_MGMT)+4]) {
        case OID_SENSOR_TYPE:
            dlen = writeGpsdoType(buffer);
            sendResults(frame, buffer, dlen);
            break;
        case OID_SENSOR_SCALE:
            dlen = writeGpsdoScale(buffer);
            sendResults(frame, buffer, dlen);
            break;
        case OID_SENSOR_PREC:
            dlen = writeGpsdoPrec(buffer);
            sendResults(frame, buffer, dlen);
            break;
        case OID_SENSOR_VALUE:
            dlen = writeGpsdoValue(buffer);
            sendResults(frame, buffer, dlen);
            break;
        case OID_SENSOR_UNITS:
            dlen = writeGpsdoUnits(buffer);
            sendResults(frame, buffer, dlen);
            break;
    }
}

int writeGpsdoType(uint8_t *buffer) {
    int dlen = 0;

    // processor temp
    dlen = writeValueInt8(
            buffer, dlen,
            OID_SENSOR_PREFIX, sizeof(OID_SENSOR_PREFIX), OID_SENSOR_TYPE,
            OID_SENSOR_TYPE_CELSIUS
    );

    // DCXO temp
    dlen = writeValueInt8(
            buffer, dlen,
            OID_SENSOR_PREFIX, sizeof(OID_SENSOR_PREFIX), OID_SENSOR_TYPE,
            OID_SENSOR_TYPE_CELSIUS
    );

    // GPSDO mean offset
    dlen = writeValueInt8(
            buffer, dlen,
            OID_SENSOR_PREFIX, sizeof(OID_SENSOR_PREFIX), OID_SENSOR_TYPE,
            OID_SENSOR_TYPE_OTHER
    );

    // GPSDO RMS offset
    dlen = writeValueInt8(
            buffer, dlen,
            OID_SENSOR_PREFIX, sizeof(OID_SENSOR_PREFIX), OID_SENSOR_TYPE,
            OID_SENSOR_TYPE_OTHER
    );

    // GPSDO RMS skew
    dlen = writeValueInt8(
            buffer, dlen,
            OID_SENSOR_PREFIX, sizeof(OID_SENSOR_PREFIX), OID_SENSOR_TYPE,
            OID_SENSOR_TYPE_OTHER
    );

    // GPSDO correction
    dlen = writeValueInt8(
            buffer, dlen,
            OID_SENSOR_PREFIX, sizeof(OID_SENSOR_PREFIX), OID_SENSOR_TYPE,
            OID_SENSOR_TYPE_OTHER
    );

    // GPSDO temperature compensation bias
    dlen = writeValueInt8(
            buffer, dlen,
            OID_SENSOR_PREFIX, sizeof(OID_SENSOR_PREFIX), OID_SENSOR_TYPE,
            OID_SENSOR_TYPE_OTHER
    );

    // GPSDO temperature compensation coefficient
    dlen = writeValueInt8(
            buffer, dlen,
            OID_SENSOR_PREFIX, sizeof(OID_SENSOR_PREFIX), OID_SENSOR_TYPE,
            OID_SENSOR_TYPE_OTHER
    );

    // GPSDO temperature compensation value
    dlen = writeValueInt8(
            buffer, dlen,
            OID_SENSOR_PREFIX, sizeof(OID_SENSOR_PREFIX), OID_SENSOR_TYPE,
            OID_SENSOR_TYPE_OTHER
    );

    return dlen;
}

int writeGpsdoScale(uint8_t *buffer) {
    int dlen = 0;

    // processor temp
    dlen = writeValueInt32(
            buffer, dlen,
            OID_SENSOR_PREFIX, sizeof(OID_SENSOR_PREFIX), OID_SENSOR_SCALE,
            OID_SENSOR_SCALE_1
    );

    // DCXO temp
    dlen = writeValueInt32(
            buffer, dlen,
            OID_SENSOR_PREFIX, sizeof(OID_SENSOR_PREFIX), OID_SENSOR_SCALE,
            OID_SENSOR_SCALE_1
    );

    // GPSDO mean offset
    dlen = writeValueInt32(
            buffer, dlen,
            OID_SENSOR_PREFIX, sizeof(OID_SENSOR_PREFIX), OID_SENSOR_SCALE,
            OID_SENSOR_SCALE_1E_9
    );

    // GPSDO RMS offset
    dlen = writeValueInt32(
            buffer, dlen,
            OID_SENSOR_PREFIX, sizeof(OID_SENSOR_PREFIX), OID_SENSOR_SCALE,
            OID_SENSOR_SCALE_1E_9
    );

    // GPSDO RMS skew
    dlen = writeValueInt32(
            buffer, dlen,
            OID_SENSOR_PREFIX, sizeof(OID_SENSOR_PREFIX), OID_SENSOR_SCALE,
            OID_SENSOR_SCALE_1E_9
    );

    // GPSDO correction
    dlen = writeValueInt32(
            buffer, dlen,
            OID_SENSOR_PREFIX, sizeof(OID_SENSOR_PREFIX), OID_SENSOR_SCALE,
            OID_SENSOR_SCALE_1
    );

    // GPSDO temperature compensation bias
    dlen = writeValueInt32(
            buffer, dlen,
            OID_SENSOR_PREFIX, sizeof(OID_SENSOR_PREFIX), OID_SENSOR_SCALE,
            OID_SENSOR_SCALE_1
    );

    // GPSDO temperature compensation coefficient
    dlen = writeValueInt32(
            buffer, dlen,
            OID_SENSOR_PREFIX, sizeof(OID_SENSOR_PREFIX), OID_SENSOR_SCALE,
            OID_SENSOR_SCALE_1
    );

    // GPSDO temperature compensation value
    dlen = writeValueInt32(
            buffer, dlen,
            OID_SENSOR_PREFIX, sizeof(OID_SENSOR_PREFIX), OID_SENSOR_SCALE,
            OID_SENSOR_SCALE_1
    );

    return dlen;
}

int writeGpsdoPrec(uint8_t *buffer) {
    int dlen = 0;

    // processor temp
    dlen = writeValueInt32(
            buffer, dlen,
            OID_SENSOR_PREFIX, sizeof(OID_SENSOR_PREFIX), OID_SENSOR_PREC,
            3
    );

    // DCXO temp
    dlen = writeValueInt32(
            buffer, dlen,
            OID_SENSOR_PREFIX, sizeof(OID_SENSOR_PREFIX), OID_SENSOR_PREC,
            3
    );

    // GPSDO mean offset
    dlen = writeValueInt32(
            buffer, dlen,
            OID_SENSOR_PREFIX, sizeof(OID_SENSOR_PREFIX), OID_SENSOR_PREC,
            1
    );

    // GPSDO RMS offset
    dlen = writeValueInt32(
            buffer, dlen,
            OID_SENSOR_PREFIX, sizeof(OID_SENSOR_PREFIX), OID_SENSOR_PREC,
            1
    );

    // GPSDO RMS skew
    dlen = writeValueInt32(
            buffer, dlen,
            OID_SENSOR_PREFIX, sizeof(OID_SENSOR_PREFIX), OID_SENSOR_PREC,
            1
    );

    // GPSDO correction
    dlen = writeValueInt32(
            buffer, dlen,
            OID_SENSOR_PREFIX, sizeof(OID_SENSOR_PREFIX), OID_SENSOR_PREC,
            4
    );

    // GPSDO temperature compensation bias
    dlen = writeValueInt32(
            buffer, dlen,
            OID_SENSOR_PREFIX, sizeof(OID_SENSOR_PREFIX), OID_SENSOR_PREC,
            4
    );

    // GPSDO temperature compensation coefficient
    dlen = writeValueInt32(
            buffer, dlen,
            OID_SENSOR_PREFIX, sizeof(OID_SENSOR_PREFIX), OID_SENSOR_PREC,
            4
    );

    // GPSDO temperature compensation value
    dlen = writeValueInt32(
            buffer, dlen,
            OID_SENSOR_PREFIX, sizeof(OID_SENSOR_PREFIX), OID_SENSOR_PREC,
            4
    );

    return dlen;
}

int writeGpsdoValue(uint8_t *buffer) {
    int dlen = 0;

    // processor temp
    dlen = writeValueInt32(
            buffer, dlen,
            OID_SENSOR_PREFIX, sizeof(OID_SENSOR_PREFIX), OID_SENSOR_VALUE,
            lroundf(TEMP_value() * 1e3f)
    );

    // DCXO temp
    dlen = writeValueInt32(
            buffer, dlen,
            OID_SENSOR_PREFIX, sizeof(OID_SENSOR_PREFIX), OID_SENSOR_VALUE,
            lroundf(TEMP_value() * 1e3f)
    );

    // GPSDO mean offset
    dlen = writeValueInt32(
            buffer, dlen,
            OID_SENSOR_PREFIX, sizeof(OID_SENSOR_PREFIX), OID_SENSOR_VALUE,
            lroundf(GPSDO_offsetMean() * 1e10f)
    );

    // GPSDO RMS offset
    dlen = writeValueInt32(
            buffer, dlen,
            OID_SENSOR_PREFIX, sizeof(OID_SENSOR_PREFIX), OID_SENSOR_VALUE,
            lroundf(GPSDO_offsetRms() * 1e10f)
    );

    // GPSDO RMS skew
    dlen = writeValueInt32(
            buffer, dlen,
            OID_SENSOR_PREFIX, sizeof(OID_SENSOR_PREFIX), OID_SENSOR_VALUE,
            lroundf(GPSDO_skewRms() * 1e10f)
    );

    // GPSDO correction
    dlen = writeValueInt32(
            buffer, dlen,
            OID_SENSOR_PREFIX, sizeof(OID_SENSOR_PREFIX), OID_SENSOR_VALUE,
            lroundf(GPSDO_freqCorr() * 1e10f)
    );

    // GPSDO temperature compensation bias
    dlen = writeValueInt32(
            buffer, dlen,
            OID_SENSOR_PREFIX, sizeof(OID_SENSOR_PREFIX), OID_SENSOR_VALUE,
            lroundf(GPSDO_compBias() * 1e10f)
    );

    // GPSDO temperature compensation coefficient
    dlen = writeValueInt32(
            buffer, dlen,
            OID_SENSOR_PREFIX, sizeof(OID_SENSOR_PREFIX), OID_SENSOR_VALUE,
            lroundf(GPSDO_compCoeff() * 1e10f)
    );

    // GPSDO temperature compensation value
    dlen = writeValueInt32(
            buffer, dlen,
            OID_SENSOR_PREFIX, sizeof(OID_SENSOR_PREFIX), OID_SENSOR_VALUE,
            lroundf(GPSDO_compValue() * 1e10f)
    );

    return dlen;
}

int writeGpsdoUnits(uint8_t *buffer) {
    int dlen = 0;

    // processor temp
    dlen = writeValueBytes(
            buffer, dlen,
            OID_SENSOR_PREFIX, sizeof(OID_SENSOR_PREFIX), OID_SENSOR_UNITS,
            "C", 1
    );

    // DCXO temp
    dlen = writeValueBytes(
            buffer, dlen,
            OID_SENSOR_PREFIX, sizeof(OID_SENSOR_PREFIX), OID_SENSOR_UNITS,
            "C", 1
    );

    // GPSDO mean offset
    dlen = writeValueBytes(
            buffer, dlen,
            OID_SENSOR_PREFIX, sizeof(OID_SENSOR_PREFIX), OID_SENSOR_UNITS,
            "s", 1
    );

    // GPSDO RMS offset
    dlen = writeValueBytes(
            buffer, dlen,
            OID_SENSOR_PREFIX, sizeof(OID_SENSOR_PREFIX), OID_SENSOR_UNITS,
            "s", 1
    );

    // GPSDO RMS skew
    dlen = writeValueBytes(
            buffer, dlen,
            OID_SENSOR_PREFIX, sizeof(OID_SENSOR_PREFIX), OID_SENSOR_UNITS,
            "s", 1
    );

    // GPSDO correction
    dlen = writeValueBytes(
            buffer, dlen,
            OID_SENSOR_PREFIX, sizeof(OID_SENSOR_PREFIX), OID_SENSOR_UNITS,
            "ppm", 1
    );

    // GPSDO temperature compensation bias
    dlen = writeValueBytes(
            buffer, dlen,
            OID_SENSOR_PREFIX, sizeof(OID_SENSOR_PREFIX), OID_SENSOR_UNITS,
            "ppm", 1
    );

    // GPSDO temperature compensation coefficient
    dlen = writeValueBytes(
            buffer, dlen,
            OID_SENSOR_PREFIX, sizeof(OID_SENSOR_PREFIX), OID_SENSOR_UNITS,
            "ppm/C", 1
    );

    // GPSDO temperature compensation value
    dlen = writeValueBytes(
            buffer, dlen,
            OID_SENSOR_PREFIX, sizeof(OID_SENSOR_PREFIX), OID_SENSOR_UNITS,
            "ppm", 1
    );

    return dlen;
}
