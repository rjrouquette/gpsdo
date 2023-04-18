//
// Created by robert on 5/21/22.
//

#ifndef GPSDO_NTP_H
#define GPSDO_NTP_H

#include <stdint.h>

void NTP_init();
void NTP_run();
uint64_t NTP_offset();
void NTP_setEpochOffset(uint32_t offset);
float NTP_clockOffset();
float NTP_clockDrift();
float NTP_clockSkew();
int NTP_clockStratum();
int NTP_leapIndicator();
float NTP_rootDelay();
float NTP_rootDispersion();
void NTP_date(uint64_t clkMono, uint32_t *ntpDate);
void NTP_setServer(int i, uint32_t addr);

char* NTP_servers(char *tail);

#endif //GPSDO_NTP_H
