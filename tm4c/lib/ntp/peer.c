//
// Created by robert on 4/27/23.
//

#include <memory.h>
#include "../chrony/candm.h"
#include "../clk/mono.h"
#include "../led.h"
#include "../net/arp.h"
#include "../net/ip.h"
#include "../net/util.h"
#include "peer.h"


#define ARP_MIN_AGE (4) // maximum polling rate
#define ARP_MAX_AGE (64) // refresh old MAC addresses


// ARP response callback
static void NtpPeer_arpCallback(void *ref, uint32_t remoteAddr, uint8_t *macAddr) {
    struct NtpPeer *this = (struct NtpPeer *) ref;
    copyMAC(this->macAddr, macAddr);
}

// Checks MAC address of peer and performs ARP request to retrieve it
static int NtpPeer_checkMac(struct NtpPeer *this) {
    uint32_t now = CLK_MONO_INT();
    uint32_t age = now - this->lastArp;
    int invalid = isNullMAC(this->macAddr) ? 1 : 0;
    if(invalid || (age > ARP_MAX_AGE)) {
        if (age > ARP_MIN_AGE) {
            if(IPv4_testSubnet(ipSubnet, ipAddress, this->source.id))
                ARP_request(ipGateway, NtpPeer_arpCallback, this);
            else
                ARP_request(this->source.id, NtpPeer_arpCallback, this);
        }
    }
    return invalid;
}

void NtpPeer_run(void *pObj) {
    struct NtpPeer *this = (struct NtpPeer *) pObj;

    // check MAC address
    if(NtpPeer_checkMac(this)) return;
}

void NtpPeer_init(void *pObj) {
    // clear structure contents
    memset(pObj, 0, sizeof(struct NtpPeer));

    struct NtpPeer *this = (struct NtpPeer *) pObj;
    // set virtual functions
    this->source.init = NtpPeer_init;
    this->source.run = NtpPeer_run;
    // set mode and state
    this->source.mode = RPY_SD_MD_CLIENT;
    this->source.state = RPY_SD_ST_UNSELECTED;
}
