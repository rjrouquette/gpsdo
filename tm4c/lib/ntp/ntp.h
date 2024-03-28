//
// Created by robert on 4/27/23.
//

#ifndef GPSDO_NTP_H
#define GPSDO_NTP_H

#ifdef __cplusplus
extern "C" {
#else
#define static_assert _Static_assert
#endif

#include <stdint.h>

#define NTP_REF_GPS (0x00535047u)

void NTP_init();

uint32_t NTP_refId();

#ifdef __cplusplus
}
#endif

#endif //GPSDO_NTP_H
