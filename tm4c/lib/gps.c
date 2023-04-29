//
// Created by robert on 5/28/22.
//

#include <memory.h>
#include "../hw/uart.h"
#include "clk/mono.h"
#include "clk/tai.h"
#include "delay.h"
#include "gps.h"

#define GPS_RING_MASK (1023)
#define GPS_RING_SIZE (1024)

static uint8_t rxBuff[GPS_RING_SIZE];
static uint8_t txBuff[GPS_RING_SIZE];
static uint8_t msgBuff[256];
static int rxHead, rxTail;
static int txHead, txTail;
static int lenNEMA, lenUBX;
static int endUBX;

static int fixGood;
static float locLat, locLon, locAlt;
static int clkBias, clkDrift, accTime, accFreq;
static volatile uint32_t taiEpoch;
static volatile int taiOffset;

static int hasNema, hasPvt, hasGpsTime, hasClock;

static void processNEMA(char *msg, int len);

static void processUBX(uint8_t *msg, int len);
static void processUbxClock(const uint8_t *payload);
static void processUbxPVT(const uint8_t *payload);
static void processUbxGpsTime(const uint8_t *payload);

static void sendUBX(uint8_t _class, uint8_t _id, int len, const uint8_t *payload);

static void configureGPS();

void GPS_init() {
    // enable GPIO A
    RCGCGPIO.EN_PORTJ = 1;
    // enable UART 3
    RCGCUART.EN_UART3 = 1;
    delay_cycles_4();

    // configure pins J0-J1
    PORTJ.LOCK = GPIO_LOCK_KEY;
    PORTJ.CR = 0x03u;
    PORTJ.DIR = 0x02u;
    PORTJ.DR8R = 0x02u;
    PORTJ.AMSEL = 0x00;
    PORTJ.AFSEL.ALT0 = 1;
    PORTJ.AFSEL.ALT1 = 1;
    PORTJ.PCTL.PMC0 = 1;
    PORTJ.PCTL.PMC1 = 1;
    PORTJ.DEN = 0x03u;
    // lock GPIO config
    PORTJ.CR = 0;
    PORTJ.LOCK = 0;

    // configure UART 3
    UART3.CTL.UARTEN = 0;
    // baud divisor = 813.80208 = (125 MHz / (16 * 9600 baud))
    UART3.IBRD.DIVINT = 813;
    UART3.FBRD.DIVFRAC = 51;
    // 8-bit data
    UART3.LCRH.WLEN = 3;
    // enable FIFO
    UART3.LCRH.FEN = 1;
    // enable UART
    UART3.CTL.RXE = 1;
    UART3.CTL.TXE = 1;
    UART3.CTL.UARTEN = 1;
}

uint32_t prevSecond = 0;
void GPS_run() {
    uint32_t now = CLK_MONO_INT();
    if(prevSecond != now) {
        prevSecond = now;
        if((now & 3) == 0)
            configureGPS();
    }

    // read GPS serial data
    while (!UART3.FR.RXFE) {
        rxBuff[rxHead++] = UART3.DR.DATA;
        rxHead &= GPS_RING_MASK;
    }

    // send GPS serial data
    while (txTail != txHead && !UART3.FR.TXFF) {
        UART3.DR.DATA = txBuff[txTail++];
        txTail &= GPS_RING_MASK;
    }

    // check for data in buffer
    if(rxTail == rxHead)
        return;
    // get next byte
    uint8_t byte = rxBuff[rxTail++];
    rxTail &= GPS_RING_MASK;

    // process UBX message
    if(lenUBX) {
        // determine end of UBX block
        if(lenUBX == 6) {
            // validate second sync byte
            if(msgBuff[1] != 0x62) {
                lenUBX = 0;
                return;
            }
            endUBX = 8 + *(uint16_t *) (msgBuff + 4);
        }
        // append character to message buffer
        if(lenUBX < sizeof(msgBuff))
            msgBuff[lenUBX] = byte;
        // end of message
        if(++lenUBX >= endUBX) {
            if(lenUBX > sizeof(msgBuff))
                lenUBX = sizeof(msgBuff);
            processUBX(msgBuff, lenUBX);
            lenUBX = 0;
            endUBX = 0;
        }
        return;
    }

    // process NEMA message
    if(lenNEMA) {
        // append character to message buffer
        if(lenNEMA < sizeof(msgBuff))
            msgBuff[lenNEMA++] = byte;
        // end of message
        if(byte == '\r' || byte == '\n') {
            msgBuff[lenNEMA--] = 0;
            processNEMA((char *) msgBuff, lenNEMA);
            // reset parser state
            lenNEMA = 0;
        }
        return;
    }

    // search for NEMA message start
    if(byte == '$') {
        msgBuff[lenNEMA++] = byte;
        return;
    }
    // search for UBX message start
    if(byte == 0xB5) {
        endUBX = 8;
        msgBuff[lenUBX++] = byte;
        return;
    }
}

float GPS_locLat() {
    return locLat;
}

float GPS_locLon() {
    return locLon;
}

float GPS_locAlt() {
    return locAlt;
}

int GPS_clkBias() {
    return clkBias;
}

int GPS_clkDrift() {
    return clkDrift;
}

int GPS_accTime() {
    return accTime;
}

int GPS_accFreq() {
    return accFreq;
}

int GPS_hasFix() {
    return fixGood;
}

static uint8_t fromHex(char c) {
    if(c < 'A') return c - '0';
    if(c < 'a') return c - 'A' + 10;
    return c - 'a' + 10;
}

static void processNEMA(char *msg, int len) {
    uint8_t csum = 0;
    char *chksum = NULL;
    char *parts[64];
    int cnt = 0;

    char *end = msg + len;
    char *ptr = msg;
    parts[cnt++] = ptr++;
    while(ptr < end) {
        char c = *ptr;
        csum ^= c;
        if(c == ',') {
            *(ptr++) = 0;
            parts[cnt++] = ptr;
            continue;
        }
        if(c == '*') {
            csum ^= c;
            *(ptr++) = 0;
            chksum = ptr;
            break;
        }
        ++ptr;
    }

    // verify checksum if present
    if(chksum) {
        uint8_t test = fromHex(chksum[0]);
        test <<= 4;
        test |= fromHex(chksum[1]);
        if(csum != test)
            return;
    }

    // no NMEA messages are expected, tell configuration check to disable them
    hasNema = 1;
}

static void processUBX(uint8_t *msg, const int len) {
    // initialize checksum
    uint8_t chkA = 0, chkB = 0;
    // compute checksum
    for(int i = 2; i < (len - 2); i++) {
        chkA += msg[i];
        chkB += chkA;
    }
    // verify checksum
    if(chkA != msg[len - 2] || chkB != msg[len - 1]) {
        return;
    }

    uint8_t _class = msg[2];
    uint8_t _id = msg[3];
    uint16_t size = *(uint16_t *)(msg+4);
    // NAV class
    if(_class == 0x01) {
        // position message
        if(_id == 0x07 && size == 92) {
            processUbxPVT(msg + 6);
            return;
        }
        // GPS time message
        if(_id == 0x20 && size == 16) {
            processUbxGpsTime(msg + 6);
            return;
        }
        // UTC time message
        if(_id == 0x22 && size == 20) {
            processUbxClock(msg + 6);
            return;
        }
    }
}

static void sendUBX(uint8_t _class, uint8_t _id, int len, const uint8_t *payload) {
    if(len > 512) return;

    // send preamble
    txBuff[txHead++] = 0xB5;
    txHead &= GPS_RING_MASK;
    txBuff[txHead++] = 0x62;
    txHead &= GPS_RING_MASK;

    // initialize checksum
    uint8_t chkA = 0, chkB = 0;

    // send class
    txBuff[txHead++] = _class;
    txHead &= GPS_RING_MASK;
    chkA += _class;
    chkB += chkA;

    // send id
    txBuff[txHead++] = _id;
    txHead &= GPS_RING_MASK;
    chkA += _id;
    chkB += chkA;

    // send length
    uint8_t *_len = (uint8_t *) & len;
    for(int i = 0; i < 2; i++) {
        txBuff[txHead++] = _len[i];
        txHead &= GPS_RING_MASK;
        chkA += _len[i];
        chkB += chkA;
    }

    // send payload
    for(int i = 0; i < len; i++) {
        txBuff[txHead++] = payload[i];
        txHead &= GPS_RING_MASK;
        chkA += payload[i];
        chkB += chkA;
    }

    // send checksum
    txBuff[txHead++] = chkA;
    txHead &= GPS_RING_MASK;
    txBuff[txHead++] = chkB;
    txHead &= GPS_RING_MASK;
}

static const uint8_t payloadDisableNMEA[] = {
        0x01, // UART port
        0x00, // reserved
        0x00, 0x00, // TX ready mode
        0xC0, 0x08, 0x00, 0x00, // UART mode (8-bit, no parity, 1 stop bit)
        0x80, 0x25, 0x00, 0x00, // 9600 baud
        0x01, 0x00, // UBX input
        0x01, 0x00, // UBX output
        0x00, 0x00, // TX timeout
        0x00, 0x00 // reserved

};
_Static_assert(sizeof(payloadDisableNMEA) == 20, "payloadDisableNMEA must be 20 bytes");

static const uint8_t payloadEnablePVT[] = {
        0x01, // NAV class
        0x07, // position, velocity, time
        0x01  // every update
};
_Static_assert(sizeof(payloadEnablePVT) == 3, "payloadEnablePVT must be 3 bytes");

static const uint8_t payloadEnableGpsTime[] = {
        0x01, // NAV class
        0x20, // GPS time
        0x01  // every update
};
_Static_assert(sizeof(payloadEnableGpsTime) == 3, "payloadEnableGpsTime must be 3 bytes");

static const uint8_t payloadEnableClock[] = {
        0x01, // NAV class
        0x22, // clock status
        0x01  // every update
};
_Static_assert(sizeof(payloadEnableClock) == 3, "payloadEnableClock must be 3 bytes");

static void configureGPS() {
    // wait for empty TX buffer
    if(txHead != txTail) return;

    // transmit configuration stanzas
    if(hasNema)
        sendUBX(0x06, 0x00, sizeof(payloadDisableNMEA), payloadDisableNMEA);
    if(!hasClock)
        sendUBX(0x06, 0x01, sizeof(payloadEnableClock), payloadEnableClock);
    if(!hasGpsTime)
        sendUBX(0x06, 0x01, sizeof(payloadEnableGpsTime), payloadEnableGpsTime);
    if(!hasPvt)
        sendUBX(0x06, 0x01, sizeof(payloadEnablePVT), payloadEnablePVT);

    hasNema = 0;
    hasClock = 0;
    hasGpsTime = 0;
    hasPvt = 0;
}

static void processUbxClock(const uint8_t *payload) {
    // clock status message received, no conf update needed
    hasClock = 1;
    // update GPS receiver clock stats
    clkBias = *(int32_t *)(payload + 4);
    clkDrift = *(int32_t *)(payload + 8);
    accTime = *(int32_t *)(payload + 12);
    accFreq = *(int32_t *)(payload + 16);
}

static void processUbxGpsTime(const uint8_t *payload) {
    // gps time received, no conf update needed
    hasGpsTime = 1;

    // data must be valid
    if((payload[11] & 3) != 3)
        return;

    // set TAI offset (GPS offset + 19s)
    taiOffset = ((signed char) payload[10]) + 19;
}

static const int lutDays365[16] = {
        0, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365
};

static const int lutDays366[16] = {
        0, 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366
};

static void processUbxPVT(const uint8_t *payload) {
    // PVT message received, no conf update needed
    hasPvt = 1;
    // fix status
    fixGood = payload[21] & 1;
    // location data
    locLon = 1e-7f * (float) *(int32_t *)(payload + 24);
    locLat = 1e-7f * (float) *(int32_t *)(payload + 28);
    locAlt = 1e-3f * (float) *(int32_t *)(payload + 36);

    // time data must be valid
    if((payload[11] & 3) != 3)
        return;

    // years
    uint32_t offset = *(uint16_t*) (payload + 4);
    offset -= 1900;
    // determine leap-year status
    int isLeap = ((offset & 3) == 0);
    if(offset % 100 == 0) isLeap = 0;
    // correct for leap-years
    uint32_t leap = offset >> 2;
    leap -= leap / 25;
    // rescale
    offset *= 365;
    offset += leap;

    // months
    offset += (isLeap ? lutDays366 : lutDays365)[payload[6]];
    // days
    offset += payload[7];
    // hours
    offset *= 24;
    offset += payload[8];
    // minutes
    offset *= 60;
    offset += payload[9];
    // seconds
    offset *= 60;
    offset += payload[10];
    // realign as TAI (PTP 1970 epoch)
    offset -= 2208988800;
    offset += taiOffset;
    // set TAI epoch
    taiEpoch = offset;
}

uint32_t GPS_taiEpoch() {
    return taiEpoch;
}

int GPS_taiOffset() {
    return taiOffset;
}
