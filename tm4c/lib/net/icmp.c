//
// Created by robert on 5/19/22.
//

#include "eth.h"
#include "icmp.h"
#include "ip.h"
#include "util.h"

void ICMP_process(uint8_t *frame, int flen) {
    struct HEADER_IPv4 *headerIPv4 = (struct HEADER_IPv4 *) (frame + sizeof(struct FRAME_ETH));

    copyIPv4(&ipGateway, headerIPv4->src);
}
