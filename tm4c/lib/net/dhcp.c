//
// Created by robert on 5/20/22.
//

#include <stdint.h>

#include "dhcp.h"
#include "../clk.h"

static volatile uint32_t dhcpLeaseExpire = 0;

void DHCP_poll() {
    uint32_t now = CLK_MONOTONIC_INT();
    if((dhcpLeaseExpire - now) < 0) {
        // re-attempt in 5 minutes if renewal fails
        dhcpLeaseExpire = now + 300;
        DHCP_renew();
    }
}

void DHCP_renew() {

}

void DHCP_release() {

}

void DHCP_process(uint8_t *frame, int len) {

}
