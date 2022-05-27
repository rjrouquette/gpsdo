//
// Created by robert on 5/24/22.
//

#ifndef GPSDO_GPSDO_H
#define GPSDO_GPSDO_H

void GPSDO_init();
void GPSDO_run();
int GPSDO_isLocked();
int GPSDO_offsetNano();
float GPSDO_getCorrection();

#endif //GPSDO_GPSDO_H
