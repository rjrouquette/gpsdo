//
// Created by robert on 4/27/23.
//

#include <memory.h>
#include "../chrony/candm.h"
#include "peer.h"

void NtpPeer_run(void *pObj) {

}

void NtpPeer_init(void *pObj) {
    // clear structure contents
    memset(pObj, 0, sizeof(struct NtpPeer));

    struct NtpPeer *this = (struct NtpPeer *) pObj;
    // set virtual functions
    this->source.init = NtpPeer_init;
    this->source.run = NtpPeer_run;
    // set mode
    this->source.mode = RPY_SD_MD_REF;
}
