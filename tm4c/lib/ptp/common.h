//
// Created by robert on 5/20/23.
//

#ifndef GPSDO_LIB_PTP_COMMON_H
#define GPSDO_LIB_PTP_COMMON_H

#include <stdint.h>

#ifndef PACKED
#define PACKED __attribute__((packed))
#endif

#define PTP2_MIN_SIZE (sizeof(HEADER_ETH) + sizeof(HEADER_PTP))
#define PTP2_VERSION (2)
#define PTP2_DOMAIN (1)


// PTP message types
enum PTP2_MTYPE {
    PTP2_MT_SYNC = 0x0,
    PTP2_MT_DELAY_REQ,
    PTP2_MT_PDELAY_REQ,
    PTP2_MT_PDELAY_RESP,

    PTP2_MT_FOLLOW_UP = 0x8,
    PTP2_MT_DELAY_RESP,
    PTP2_MT_PDELAY_FOLLOW_UP,
    PTP2_MT_ANNOUNCE,
    PTP2_MT_SIGNALING,
    PTP2_MT_MANAGEMENT
};

enum PTP2_CLK_CLASS {
    PTP2_CLK_CLASS_PRIMARY = 6,
    PTP2_CLK_CLASS_PRI_HOLD = 7,
    PTP2_CLK_CLASS_PRI_FAIL = 52
};

// PTP time source
enum PTP2_TSRC {
    PTP2_TSRC_ATOMIC = 0x10,
    PTP2_TSRC_GPS = 0x20,
    PTP2_TSRC_RADIO = 0x30,
    PTP2_TSRC_PTP = 0x40,
    PTP2_TSRC_NTP = 0x50,
    PTP2_TSRC_MANUAL = 0x60,
    PTP2_TSRC_OTHER = 0x90,
    PTP2_TSRC_INTERNAL = 0xA0
};

typedef struct PACKED PTP2_SRC_IDENT {
        uint8_t identity[8];
        uint16_t portNumber;
} PTP2_SRC_IDENT;
_Static_assert(sizeof(struct PTP2_SRC_IDENT) == 10, "PTP2_SRC_IDENT must be 10 bytes");

typedef struct PACKED PTP2_TIMESTAMP {
        uint16_t secondsHi;
        uint32_t secondsLo;
        uint32_t nanoseconds;
} PTP2_TIMESTAMP;
_Static_assert(sizeof(struct PTP2_TIMESTAMP) == 10, "PTP2_TIMESTAMP must be 10 bytes");

typedef struct PACKED HEADER_PTP {
        uint16_t messageType: 4;        // lower nibble
        uint16_t transportSpecific: 4;  // upper nibble
        uint16_t versionPTP: 4;         // lower nibble
        uint16_t reserved0: 4;          // upper nibble
        uint16_t messageLength;
        uint8_t domainNumber;
        uint8_t reserved1;
        uint16_t flags;
        uint64_t correctionField;
        uint32_t reserved2;
        PTP2_SRC_IDENT sourceIdentity;
        uint16_t sequenceId;
        uint8_t controlField;
        uint8_t logMessageInterval;
} HEADER_PTP;
_Static_assert(sizeof(struct HEADER_PTP) == 34, "HEADER_PTP must be 34 bytes");

typedef struct PACKED PTP2_ANNOUNCE {
        PTP2_TIMESTAMP originTimestamp;
        uint16_t currentUtcOffset;
        uint8_t reserved;
        uint8_t grandMasterPriority;
        uint32_t grandMasterClockQuality;
        uint8_t grandMasterPriority2;
        uint8_t grandMasterIdentity[8];
        uint16_t stepsRemoved;
        uint8_t timeSource;
} PTP2_ANNOUNCE;
_Static_assert(sizeof(struct PTP2_ANNOUNCE) == 30, "PTP2_ANNOUNCE must be 34 bytes");

typedef struct PACKED PTP2_DELAY_RESP {
        PTP2_TIMESTAMP receiveTimestamp;
        PTP2_SRC_IDENT requestingIdentity;
} PTP2_DELAY_RESP;
_Static_assert(sizeof(struct PTP2_DELAY_RESP) == 20, "PTP2_DELAY_RESP must be 34 bytes");

typedef struct PACKED PTP2_PDELAY_REQ {
        PTP2_TIMESTAMP receiveTimestamp;
        uint8_t reserved[10];
} PTP2_PDELAY_REQ;
_Static_assert(sizeof(struct PTP2_PDELAY_REQ) == 20, "PTP2_PDELAY_REQ must be 34 bytes");

typedef struct PACKED PTP2_PDELAY_RESP {
        PTP2_TIMESTAMP receiveTimestamp;
        PTP2_SRC_IDENT requestingIdentity;
} PTP2_PDELAY_RESP;
_Static_assert(sizeof(struct PTP2_PDELAY_RESP) == 20, "PTP2_PDELAY_RESP must be 34 bytes");

typedef struct PACKED PTP2_PDELAY_FOLLOW_UP {
        PTP2_TIMESTAMP responseTimestamp;
        PTP2_SRC_IDENT requestingIdentity;
} PTP2_PDELAY_FOLLOW_UP;
_Static_assert(sizeof(struct PTP2_PDELAY_RESP) == 20, "PTP2_PDELAY_FOLLOW_UP must be 34 bytes");


// lut for PTP clock accuracy codes
extern const float lutClkAccuracy[17];

// IEEE 802.1AS broadcast MAC address (01:80:C2:00:00:0E)
extern const uint8_t gPtpMac[6];

// ID for the local clock
extern uint8_t ptpClockId[8];

/**
 * Convert fixed-point 64-bit timestamp to PTP timestamp
 * @param ts fixed-point timestamp
 * @param tsPtp PTP timestamp
 */
void toPtpTimestamp(uint64_t ts, PTP2_TIMESTAMP *tsPtp);

/**
 * Convert PTP timestamp to fixed-point 64-bit timestamp
 * @param tsPtp PTP timestamp
 * @return fixed-point 64-bit timestamp
 */
uint64_t fromPtpTimestamp(PTP2_TIMESTAMP *tsPtp);

/**
 * Translate clock RMS error into PTP accuracy code
 * @param rmsError
 * @return PTP accuracy code
 */
uint32_t toPtpClkAccuracy(float rmsError);


#endif //GPSDO_LIB_PTP_COMMON_H
