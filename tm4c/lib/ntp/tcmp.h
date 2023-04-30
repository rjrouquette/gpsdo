//
// Created by robert on 4/30/23.
//

#ifndef GPSDO_TCMP_H
#define GPSDO_TCMP_H

#include <stdint.h>

void TCMP_init();

void TCMP_run();

/**
 * Get the current temperature measurement
 * @return the current temperature in Celsius
 */
float TCMP_temp();

float TCMP_get();

void TCMP_update(float target);

#endif //GPSDO_TCMP_H
