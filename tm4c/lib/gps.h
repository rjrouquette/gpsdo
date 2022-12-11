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

int GPS_hasLock();

#endif //GPSDO_GPS_H
