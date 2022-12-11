//
// Created by robert on 5/24/22.
//

#include "lib/clk.h"
#include "gpsdo.h"
#include "ntp.h"


const uint8_t OID_NTP_INFO_PREFIX[] = { 0x06, 0x0A, 0x2B, 6, 1, 2, 1, 0x81, 0x45, 1, 1 };
#define NTP_INFO_SOFT_NAME (1)
#define NTP_INFO_SOFT_VERS (2)
#define NTP_INFO_SOFT_VEND (3)
#define NTP_INFO_SYS_TYPE (4)
#define NTP_INFO_TIME_RES (5)
#define NTP_INFO_TIME_PREC (6)
#define NTP_INFO_TIME_DIST (7)

int writeNtpInfo(uint8_t *buffer) {
    int dlen = 0;
    dlen = writeValueBytes(
            buffer, dlen,
            OID_NTP_INFO_PREFIX, sizeof(OID_NTP_INFO_PREFIX), NTP_INFO_SOFT_NAME,
            "GPSDO.NTP",  9
    );
    dlen = writeValueBytes(
            buffer, dlen,
            OID_NTP_INFO_PREFIX, sizeof(OID_NTP_INFO_PREFIX), NTP_INFO_SOFT_VERS,
            VERSION_GIT,  8
    );
    dlen = writeValueBytes(
            buffer, dlen,
            OID_NTP_INFO_PREFIX, sizeof(OID_NTP_INFO_PREFIX), NTP_INFO_SOFT_VEND,
            "N/A",  3
    );
    dlen = writeValueBytes(
            buffer, dlen,
            OID_NTP_INFO_PREFIX, sizeof(OID_NTP_INFO_PREFIX), NTP_INFO_SYS_TYPE,
            "TM4C1294NCPDT",  13
    );
    dlen = writeValueInt32(
            buffer, dlen,
            OID_NTP_INFO_PREFIX, sizeof(OID_NTP_INFO_PREFIX), NTP_INFO_TIME_RES,
            25000000
    );
    dlen = writeValueInt8(
            buffer, dlen,
            OID_NTP_INFO_PREFIX, sizeof(OID_NTP_INFO_PREFIX), NTP_INFO_TIME_PREC,
            -24
    );
    dlen = writeValueBytes(
            buffer, dlen,
            OID_NTP_INFO_PREFIX, sizeof(OID_NTP_INFO_PREFIX), NTP_INFO_TIME_DIST,
            "40 ns",  5
    );
    return dlen;
}


const uint8_t OID_NTP_STATUS_PREFIX[] = { 0x06, 0x0A, 0x2B, 6, 1, 2, 1, 0x81, 0x45, 1, 2 };
#define NTP_STATUS_CURR_MODE (1)
#define NTP_STATUS_STRATUM (2)
#define NTP_STATUS_ACTV_ID (3)
#define NTP_STATUS_ACTV_NAME (4)
#define NTP_STATUS_ACTV_OFFSET (5)
#define NTP_STATUS_NUM_REFS (6)
#define NTP_STATUS_ROOT_DISP (7)
#define NTP_STATUS_UPTIME (8)
#define NTP_STATUS_NTP_DATE (9)

int writeNtpStatus(uint8_t *buffer) {
    int isLocked = GPSDO_isLocked();

    int dlen = 0;
    dlen = writeValueInt8(
            buffer, dlen,
            OID_NTP_STATUS_PREFIX, sizeof(OID_NTP_STATUS_PREFIX), NTP_STATUS_CURR_MODE,
            isLocked ? 5 : 2
    );
    dlen = writeValueInt8(
            buffer, dlen,
            OID_NTP_STATUS_PREFIX, sizeof(OID_NTP_STATUS_PREFIX), NTP_STATUS_STRATUM,
            isLocked ? 1 : 16
    );
    dlen = writeValueInt8(
            buffer, dlen,
            OID_NTP_STATUS_PREFIX, sizeof(OID_NTP_STATUS_PREFIX), NTP_STATUS_ACTV_ID,
            0
    );
    dlen = writeValueBytes(
            buffer, dlen,
            OID_NTP_STATUS_PREFIX, sizeof(OID_NTP_STATUS_PREFIX), NTP_STATUS_ACTV_NAME,
            "GPS", 3
    );

    char temp[16];
    int end = fmtFloat((float) GPSDO_offsetNano(), 0, 0, temp);
    strcpy(temp+end, " ns");
    dlen = writeValueBytes(
            buffer, dlen,
            OID_NTP_STATUS_PREFIX, sizeof(OID_NTP_STATUS_PREFIX), NTP_STATUS_ACTV_OFFSET,
            temp, end + 3
    );

    dlen = writeValueInt8(
            buffer, dlen,
            OID_NTP_STATUS_PREFIX, sizeof(OID_NTP_STATUS_PREFIX), NTP_STATUS_NUM_REFS,
            1
    );
    dlen = writeValueInt32(
            buffer, dlen,
            OID_NTP_STATUS_PREFIX, sizeof(OID_NTP_STATUS_PREFIX), NTP_STATUS_ROOT_DISP,
            0
    );
    dlen = writeValueInt32(
            buffer, dlen,
            OID_NTP_STATUS_PREFIX, sizeof(OID_NTP_STATUS_PREFIX), NTP_STATUS_UPTIME,
            CLK_MONOTONIC_INT()
    );

    uint32_t ntpDate[4];
    NTP_date(CLK_GPS(), ntpDate);
    dlen = writeValueBytes(
            buffer, dlen,
            OID_NTP_STATUS_PREFIX, sizeof(OID_NTP_STATUS_PREFIX), NTP_STATUS_NTP_DATE,
            ntpDate, sizeof(ntpDate)
    );
    return dlen;
}

void sendNTP(uint8_t *frame) {
    // variable bindings
    uint8_t buffer[512];
    int dlen;

    // filter MIB
    if(buffOID[sizeof(OID_PREFIX_MGMT)+2] != 1)
        return;

    // select output data
    switch (buffOID[sizeof(OID_PREFIX_MGMT)+3]) {
        case 1:
            dlen = writeNtpInfo(buffer);
            sendResults(frame, buffer, dlen);
            break;
        case 2:
            dlen = writeNtpStatus(buffer);
            sendResults(frame, buffer, dlen);
            break;
    }
}
