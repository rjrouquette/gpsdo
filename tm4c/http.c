//
// Created by robert on 5/21/22.
//

#include <memory.h>

#include "lib/net/eth.h"
#include "lib/net/ip.h"
#include "lib/net/tcp.h"
#include "lib/net/util.h"

#include "http.h"

#define MAX_CONNS (4)
struct TCP_CONN tcpConn[MAX_CONNS];


void HTTP_process(uint8_t *frame, int flen);


void HTTP_init() {
    // initialize connections
    memset(tcpConn, 0, sizeof(tcpConn));
    // register HTTP server port
    TCP_register(80, HTTP_process);
}

void HTTP_poll() {

}

extern volatile uint8_t debugMac[6];
void HTTP_process(uint8_t *frame, int flen) {
    // map headers
    struct FRAME_ETH *headerEth = (struct FRAME_ETH *) frame;
    struct HEADER_IPv4 *headerIPv4 = (struct HEADER_IPv4 *) (headerEth + 1);
    struct HEADER_TCP *headerTCP = (struct HEADER_TCP *) (headerIPv4 + 1);

    // verify destination
    if(headerIPv4->dst != ipAddress) return;

    // delegate processing to connection instance
    TCP_delegate(frame, flen, tcpConn, MAX_CONNS);
}
