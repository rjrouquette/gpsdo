//
// Created by robert on 5/29/22.
//

#ifndef GPSDO_TCOMP_H
#define GPSDO_TCOMP_H

void TCOMP_init();
float TCOMP_temperature();

void TCOMP_updateTarget(float target);
float TCOMP_getComp(float *m, float *b);

#endif //GPSDO_TCOMP_H
