//
// Created by robert on 4/27/23.
//

#ifndef GPSDO_REF_H
#define GPSDO_REF_H

#include "src.h"

struct NtpGPS {
    struct NtpSource source;

    uint64_t last_update;
    uint64_t last_pps;
};

void NtpGPS_init(void *pObj);

#endif //GPSDO_REF_H
