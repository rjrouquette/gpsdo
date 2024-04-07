//
// Created by robert on 4/27/23.
//

#ifndef GPSDO_NTP_H
#define GPSDO_NTP_H

#include <cstdint>

#define NTP_REF_GPS (0x00535047u)

namespace ntp {
    void init();

    uint32_t refId();
}

#endif //GPSDO_NTP_H
