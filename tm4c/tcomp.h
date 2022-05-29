//
// Created by robert on 5/29/22.
//

#ifndef GPSDO_TCOMP_H
#define GPSDO_TCOMP_H

void TCOMP_init();
void TCOMP_update(float temp, float comp);
void TCOMP_getCoeff(float temp, float *m, float *b);

#endif //GPSDO_TCOMP_H
