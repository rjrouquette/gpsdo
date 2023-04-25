/**
 * Stripped down version of TinyMT
 * https://github.com/MersenneTwister-Lab/TinyMT
 */

#ifndef GPSDO_RAND_H
#define GPSDO_RAND_H

#include <stdint.h>

void RAND_init();

uint32_t RAND_next();

#endif //GPSDO_RAND_H
