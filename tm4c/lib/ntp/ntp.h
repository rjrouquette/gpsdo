//
// Created by robert on 4/27/23.
//

#ifndef GPSDO_NTP_H
#define GPSDO_NTP_H

#define NTP_REF_GPS (0x00535047u)

void NTP_init();

void NTP_run();

uint32_t NTP_refId();

#endif //GPSDO_NTP_H
