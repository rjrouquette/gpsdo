//
// Created by robert on 5/20/22.
//

#ifndef GPSDO_TCP_H
#define GPSDO_TCP_H

#include <stdint.h>

#ifndef PACKED
#define PACKED __attribute__((packed))
#endif

struct PACKED HEADER_TCP {
    uint8_t portSrc[2];
    uint8_t portDst[2];
    uint8_t seq[4];
    uint8_t ack[4];
    union PACKED {
        struct PACKED {
            unsigned NS: 1;
            unsigned : 3;
            unsigned offset: 4;
            unsigned FIN: 1;
            unsigned SYN: 1;
            unsigned RST: 1;
            unsigned PSH: 1;
            unsigned ACK: 1;
            unsigned URG: 1;
            unsigned ECE: 1;
            unsigned CWR: 1;
        };
        uint16_t bits;
    } flags;
    uint8_t window[2];
    uint8_t chksum[2];
    uint8_t urgent[2];
};
_Static_assert(sizeof(struct HEADER_TCP) == 20, "HEADER_TCP must be 20 bytes");

typedef void (*CallbackTCP)(uint8_t *frame, int flen);

typedef void (*CallbackTcpRx)(int conn, uint32_t seq, uint8_t *data, int len);

struct TCP_CONN {
    union {
        struct {
            unsigned inuse: 1;
            unsigned syn: 1;
            unsigned active: 1;
            unsigned waitSend: 1;
            unsigned fin: 1;
        };
        uint32_t bits;
    } state;
    // local and remote address
    struct {
        uint32_t ip4;
        uint16_t port;
        uint8_t mac[6];
    } addrLocal, addrRemote;
    uint32_t seqRX;
    uint32_t seqTX;
    uint32_t timer;
    CallbackTcpRx callbackRx;
};


void TCP_process(uint8_t *frame, int flen);
void TCP_finalize(uint8_t *frame, int flen);

int TCP_register(uint16_t port, CallbackTCP callback);
int TCP_deregister(uint16_t port);

void TCP_delegate(uint8_t *frame, int flen, struct TCP_CONN *tcpConn, int cntConn);
int TCP_CONN_send(struct TCP_CONN *tcpConn, void *data, int len);

#endif //GPSDO_TCP_H
