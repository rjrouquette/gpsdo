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
typedef volatile struct NtpGPS NtpGPS;

void NtpGPS_init(volatile void *pObj);

#endif //GPSDO_REF_H
