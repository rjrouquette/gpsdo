#ifndef GPSDO_RAND_H
#define GPSDO_RAND_H

#ifdef __cplusplus
extern "C" {
#else
#define static_assert _Static_assert
#endif

#include <stdint.h>

void RAND_init();

uint32_t RAND_next();

#ifdef __cplusplus
}
#endif

#endif //GPSDO_RAND_H
