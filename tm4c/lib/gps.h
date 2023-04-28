//
// Created by robert on 5/28/22.
//

#ifndef GPSDO_GPS_H
#define GPSDO_GPS_H


void GPS_init();
void GPS_run();

float GPS_locLat();
float GPS_locLon();
float GPS_locAlt();

int GPS_clkBias();
int GPS_clkDrift();
int GPS_accTime();
int GPS_accFreq();

int GPS_hasFix();

uint32_t GPS_taiEpoch();
int GPS_taiOffset();

#endif //GPSDO_GPS_H
