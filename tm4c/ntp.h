//
// Created by robert on 5/21/22.
//

#ifndef GPSDO_NTP_H
#define GPSDO_NTP_H

#include <stdint.h>

#define NTP_UTC_OFFSET (2208988800)

void NTP_init();
void NTP_run();
uint64_t NTP_offset();
void NTP_date(uint64_t clkMono, uint32_t *ntpDate);

#endif //GPSDO_NTP_H
