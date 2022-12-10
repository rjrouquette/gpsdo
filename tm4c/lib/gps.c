//
// Created by robert on 5/28/22.
//

#include <memory.h>
#include "../hw/uart.h"
#include "delay.h"
#include "gps.h"
#include "format.h"
#include "clk.h"

#define GPS_RING_MASK (1023)
#define GPS_RING_SIZE (1024)

static int ptrDebug;
static char debug[32][44];

static uint8_t rxBuff[GPS_RING_SIZE];
static uint8_t txBuff[GPS_RING_SIZE];
static uint8_t msgBuff[256];
static int rxHead, rxTail;
static int txHead, txTail;
static int lenNEMA, lenUBX;
static int endUBX;

static float locLat, locLon, locAlt;

static void debugLog(const char *msg) {
    ++ptrDebug ;
    ptrDebug &= 31;
    strncpy(debug[ptrDebug], msg, 44);
}

static void processNEMA(char *msg, int len);

static void processUBX(uint8_t *msg, int len);
static void sendUBX(uint8_t _class, uint8_t _id, int len, const uint8_t *payload);

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

    debugLog("initialized GPS UART for 9600 baud");
}

uint32_t prevSecond = 0;
void GPS_run() {
    uint32_t now = CLK_MONOTONIC_INT();
    if(prevSecond != now) {
        prevSecond = now;
        char sep[20] = "---- #### ----";
        toDec(now % 10000, 4, '0', sep + 5);
        debugLog(sep);

        uint8_t ubx[] = { 0 };
        sendUBX(0x06, 0x02, sizeof(ubx), ubx);
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

char* GPS_log(char *tail) {
    char line[48];
    int ptr = (ptrDebug + 1) & 31;
    for(int i = 0; i < 32; i++) {
        strncpy(line, debug[ptr], 44);
        line[44] = 0;
        tail = append(tail, line);
        tail = append(tail, "\n");
        ptr = (ptr + 1) & 31;
    }
    return tail;
}

int GPS_hasLock() {
    return 1;
}

static uint8_t fromHex(char c) {
    if(c < 'A') return c - '0';
    if(c < 'a') return c - 'A' + 10;
    return c - 'a' + 10;
}

static void processNEMA(char *msg, int len) {
//    debugLog(msg);

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
        if(csum != test) {
            char tmp[44];
            char *tail = append(tmp, "NEMA checksum error: ");
            tail = append(tail, msg);
            *tail = 0;
            debugLog(tmp);
            return;
        }
    }

    // TODO process NEMA messages
}

static void processUBX(uint8_t *msg, const int len) {
    char tmp[44];

    // initialize checksum
    uint8_t chkA = 0, chkB = 0;
    // compute checksum
    for(int i = 2; i < (len - 2); i++) {
        chkA += msg[i];
        chkB += chkA;
    }
    // verify checksum
    if(chkA != msg[len - 2] || chkB != msg[len - 1]) {
        debugLog("UBX checksum error:");
        for(int i = 0; i < len; i += 8) {
            char *tail = append(tmp, "  ");
            if(len - i < 8)
                tail = toHexBytes(tail, msg + i, len - i);
            else
                tail = toHexBytes(tail, msg + i, 8);
            *tail = 0;
            debugLog(tmp);
        }
        return;
    }

    uint8_t _class = msg[2];
    uint8_t _id = msg[3];
    uint16_t size = *(uint16_t *)(msg+4);
    // ack/nack
    if(_class == 0x05) {
        if(size != 2) return;
        char *tail;
        if(_id == 1)
            tail = append(tmp, "UBX ACK: ");
        else
            tail = append(tmp, "UBX NACK: ");
        tail = toHexBytes(tail, msg + 6, 2);
        *tail = 0;
        debugLog(tmp);
        return;
    }

    debugLog("UBX MSG:");
    for(int i = 0; i < len; i += 8) {
        char *tail = append(tmp, "  ");
        if(len - i < 8)
            tail = toHexBytes(tail, msg + i, len - i);
        else
            tail = toHexBytes(tail, msg + i, 8);
        *tail = 0;
        debugLog(tmp);
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
