//
// Created by robert on 5/28/22.
//

#include <memory.h>
#include <stdbool.h>
#include "../hw/interrupts.h"
#include "../hw/uart.h"
#include "clk/clk.h"
#include "clk/mono.h"
#include "clk/util.h"
#include "delay.h"
#include "gps.h"

#include <math.h>

#include "run.h"

#define GPS_RING_SIZE (256)
#define GPS_RING_MASK (GPS_RING_SIZE - 1)

#define GPS_RST_PORT (PORTP)
#define GPS_RST_PIN (1<<5)
#define GPS_RST_INTV (300)
#define GPS_RST_THR (60)

#define INTV_HEALTH (RUN_SEC >> 1)

#define ADV_RING(ptr) ((ptr) = ((ptr) + 1) & GPS_RING_MASK)

static uint8_t rxBuff[GPS_RING_SIZE];
static uint8_t txBuff[GPS_RING_SIZE];
static uint8_t msgBuff[256];
static volatile int rxHead, rxTail;
static volatile int txHead, txTail;
static volatile int lenNEMA, lenUBX;
static volatile int endUBX;

static void * volatile taskParse;

static volatile uint32_t lastReset;
static volatile int fixGood;
static volatile float locLat, locLon, locAlt;
static volatile int clkBias, clkDrift, accTime, accFreq;
static volatile uint64_t taiEpochUpdate;
static volatile uint32_t taiEpoch;
static volatile int taiOffset = 37; // as of 2016-12-31

static volatile bool hasNema, hasPvt, hasGpsTime, hasClock, wasReset;

__attribute__((noinline))
static void processNEMA(char *msg, int len);

__attribute__((noinline))
static void processUBX(uint8_t *msg, int len);
static void processUbxClock(const uint8_t *payload);
static void processUbxPVT(const uint8_t *payload);
static void processUbxGpsTime(const uint8_t *payload);

__attribute__((noinline))
static void sendUBX(uint8_t _class, uint8_t _id, int len, const uint8_t *payload);

static void configureGPS();

static void runHealth(void *ref);
static void runParse(void *ref);

static void startTx();

void GPS_init() {
    // enable GPIO J
    RCGCGPIO.EN_PORTJ = 1;
    // enable GPIO P
    RCGCGPIO.EN_PORTP = 1;
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

    // configure reset pins
    GPS_RST_PORT.LOCK = GPIO_LOCK_KEY;
    GPS_RST_PORT.CR = GPS_RST_PIN;
    GPS_RST_PORT.DIR = GPS_RST_PIN;
    GPS_RST_PORT.DR8R = GPS_RST_PIN;
    GPS_RST_PORT.DEN = GPS_RST_PIN;
    // lock GPIO config
    GPS_RST_PORT.CR = 0;
    GPS_RST_PORT.LOCK = 0;

    // configure UART 3
    UART3.CTL.UARTEN = 0;
    // baud divisor
    const float divisor = CLK_FREQ / (16.0f * 9600.0f);
    const int divisorInt = (int) divisor;
    const int divisorFrac = (int) ((divisor - divisorInt) *  64.0f);
    UART3.IBRD.DIVINT = divisorInt;
    UART3.FBRD.DIVFRAC = divisorFrac;
    // 8-bit data
    UART3.LCRH.WLEN = 3;
    // enable FIFO
    UART3.LCRH.FEN = 1;
    // enable UART
    UART3.CTL.RXE = 1;
    UART3.CTL.TXE = 1;
    UART3.CTL.UARTEN = 1;
    // start parser thread
    taskParse = runSleep(RUN_MAX, runParse, NULL);
    // reduce interrupt priority
    ISR_priority(ISR_UART3, 7);
    // enable interrupts
    UART3.IM.RT = 1;
    UART3.IM.RX = 1;

    // reset GPS module
    GPS_RST_PORT.DATA[GPS_RST_PIN] = 0;
    // start health check thread
    wasReset = true;
    runSleep(INTV_HEALTH, runHealth, NULL);
}

static void runHealth(void *ref) {
    // check GPS message configuration
    configureGPS();

    // release reset pin
    if(GPS_RST_PORT.DATA[GPS_RST_PIN]) {
        GPS_RST_PORT.DATA[GPS_RST_PIN] = GPS_RST_PIN;
        wasReset = true;
    }

    // reset GPS if PPS has ceased
    uint32_t now = CLK_MONO_INT();
    if((now - lastReset) > GPS_RST_INTV) {
        union fixed_32_32 age;
        age.full = CLK_MONO();
        uint64_t pps[3];
        CLK_PPS(pps);
        age.full -= pps[0];
        if (age.ipart > GPS_RST_THR) {
            // reset GPS
            GPS_RST_PORT.DATA[GPS_RST_PIN] = 0;
            lastReset = now;
        }
    }
}

void ISR_UART3() {
    const uint32_t flags = UART3.MIS.raw;

    // RX FIFO watermark or RX timeout
    if(flags & 0x50) {
        // drain UART FIFO (clears interrupt)
        int head = rxHead;
        while (!UART3.FR.RXFE) {
            rxBuff[head] = UART3.DR.DATA;
            ADV_RING(head);
        }
        rxHead = head;
        // wake parser
        runWake(taskParse);
    }

    // TX FIFO watermark
    if(flags & 0x20) {
        // fill UART FIFO
        const int head = txHead;
        int tail = txTail;
        while ((!UART3.FR.TXFF) && (tail != head)) {
            UART3.DR.DATA = txBuff[tail];
            ADV_RING(tail);
        }
        txTail = tail;
        // explicitly clear flag
        UART3.ICR.TX = 1;
    }
}

static void startTx() {
    // disable interrupt to prevent clobbering of pointers
    UART3.IM.TX = 0;
    // fill UART FIFO
    const int head = txHead;
    int tail = txTail;
    while ((!UART3.FR.TXFF) && (tail != head)) {
        UART3.DR.DATA = txBuff[tail];
        ADV_RING(tail);
    }
    txTail = tail;
    // enable interrupt to complete any deferred bytes
    UART3.IM.TX = 1;
}

static void runParse(void *ref) {
    int tail = rxTail;
    while(tail != rxHead) {
        // get next byte
        uint8_t byte = rxBuff[tail];
        ADV_RING(tail);

        // process UBX message
        if(lenUBX) {
            // append character to message buffer
            if(lenUBX < sizeof(msgBuff))
                msgBuff[lenUBX] = byte;

            // end of message
            if(++lenUBX >= endUBX) {
                if(lenUBX > sizeof(msgBuff))
                    lenUBX = sizeof(msgBuff);
                processUBX((uint8_t *) msgBuff, lenUBX);
                lenUBX = 0;
                endUBX = 0;
                continue;
            }

            // parse UBX message length
            if(lenUBX == 6) {
                endUBX = 8 + *(uint16_t *) (msgBuff + 4);
                // resync if message length is bad
                if(endUBX > sizeof(msgBuff))
                    lenUBX = 0;
                continue;
            }

            // validate second sync byte
            if(lenUBX == 2 && msgBuff[1] != 0x62)
                lenUBX = 0;
            continue;
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
        }

        // check for NEMA message start
        if(byte == '$') {
            msgBuff[0] = byte;
            lenNEMA = 1;
            continue;
        }

        // check for UBX message start
        if(byte == 0xB5) {
            lenNEMA = 0;
            msgBuff[0] = byte;
            lenUBX = 1;
            endUBX = 8;
            continue;
        }
    }
    rxTail = tail;
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

static void processNEMA(char *ptr, const int len) {
    uint8_t csum = 0;
    const char *chksum = NULL;

    const char *const end = ptr + len;
    while(++ptr < end) {
        const char c = *ptr;
        csum ^= c;
        if(c == '*') {
            csum ^= c;
            chksum = ++ptr;
            break;
        }
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
    hasNema = true;
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
    int head = txHead;

    // send preamble
    txBuff[head] = 0xB5;
    ADV_RING(head);
    txBuff[head] = 0x62;
    ADV_RING(head);

    // initialize checksum
    uint8_t chkA = 0, chkB = 0;

    // send class
    txBuff[head] = _class;
    ADV_RING(head);
    chkA += _class;
    chkB += chkA;

    // send id
    txBuff[head] = _id;
    ADV_RING(head);
    chkA += _id;
    chkB += chkA;

    // send length
    uint8_t *_len = (uint8_t *) & len;
    for(int i = 0; i < 2; i++) {
        txBuff[head] = _len[i];
        ADV_RING(head);
        chkA += _len[i];
        chkB += chkA;
    }

    // send payload
    for(int i = 0; i < len; i++) {
        txBuff[head] = payload[i];
        ADV_RING(head);
        chkA += payload[i];
        chkB += chkA;
    }

    // send checksum
    txBuff[head] = chkA;
    ADV_RING(head);
    txBuff[head] = chkB;
    ADV_RING(head);

    txHead = head;
    startTx();
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

static const uint8_t payloadNavConfig[] = {
    0x01, 0x02, // mask (dynamic model, UTC reference)
    0x02, // stationary mode
    0x02, // 3D fix mode
    0x00, 0x00, 0x00, 0x00, // fixed altitude (not set)
    0x00, 0x00, 0x00, 0x00, // fixed altitude variance (not set)
    0x00, // minimum satellite elevation degrees (not set)
    0x00, // reserved (drLimit)
    0x00, 0x00, // position DOP (not set)
    0x00, 0x00, // time DOP (not set)
    0x00, 0x00, // position accuracy (not set)
    0x00, 0x00, // time accuracy (not set)
    0x00, // static hold threshold (not set)
    0x00, // DGNSS timeout (not set)
    0x00, // cnoThreshNumSVs timeout (not set)
    0x00, // cnoThresh (not set)
    0x00, 0x00, // reserved
    0x00, 0x00, // static hold maximum distance (not set)
    0x03, // UTC USNO
    0x00, 0x00, 0x00, 0x00, 0x00 // reserved
};
_Static_assert(sizeof(payloadNavConfig) == 36, "payloadNavConfig must be 3 bytes");

static void configureGPS() {
    // transmit configuration stanzas
    if(hasNema)
        sendUBX(0x06, 0x00, sizeof(payloadDisableNMEA), payloadDisableNMEA);
    if(!hasClock)
        sendUBX(0x06, 0x01, sizeof(payloadEnableClock), payloadEnableClock);
    if(!hasGpsTime)
        sendUBX(0x06, 0x01, sizeof(payloadEnableGpsTime), payloadEnableGpsTime);
    if(!hasPvt)
        sendUBX(0x06, 0x01, sizeof(payloadEnablePVT), payloadEnablePVT);
    if(wasReset)
        sendUBX(0x06, 0x24, sizeof(payloadDisableNMEA), payloadDisableNMEA);

    hasNema = false;
    hasClock = false;
    hasGpsTime = false;
    hasPvt = false;
    wasReset = false;
}

static void processUbxClock(const uint8_t *payload) {
    // clock status message received, no conf update needed
    hasClock = true;
    // update GPS receiver clock stats
    clkBias = *(int32_t *)(payload + 4);
    clkDrift = *(int32_t *)(payload + 8);
    accTime = *(int32_t *)(payload + 12);
    accFreq = *(int32_t *)(payload + 16);
}

static void processUbxGpsTime(const uint8_t *payload) {
    // gps time received, no conf update needed
    hasGpsTime = true;

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
    hasPvt = true;
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
    int isLeap = ((offset & 3) == 0) ? 1 : 0;
    if(offset % 100 == 0) isLeap = 0;
    // correct for leap-years
    uint32_t leap = offset >> 2;
    leap -= leap / 25;
    leap -= isLeap;
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
    // mark as updated
    if(taiOffset != 0)
        taiEpochUpdate = CLK_MONO();
}

uint64_t GPS_taiEpochUpdate() {
    return taiEpochUpdate;
}

uint32_t GPS_taiEpoch() {
    return taiEpoch;
}

int GPS_taiOffset() {
    return taiOffset;
}
