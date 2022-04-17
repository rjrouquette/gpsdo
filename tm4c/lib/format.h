//
// Created by robert on 4/17/22.
//

#ifndef GPSDO_FORMAT_H
#define GPSDO_FORMAT_H

int toBin(unsigned value, int width, char padding, char *origin);
int toOct(unsigned value, int width, char padding, char *origin);
int toDec(unsigned value, int width, char padding, char *origin);
int toHex(unsigned value, int width, char padding, char *origin);

#endif //GPSDO_FORMAT_H
