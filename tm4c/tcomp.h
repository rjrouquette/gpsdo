//
// Created by robert on 5/29/22.
//

#ifndef GPSDO_TCOMP_H
#define GPSDO_TCOMP_H

void TCOMP_init();
void TCOMP_update(const float *temp, float target);
void TCOMP_getCoeff(const float *temp, float *m, float *b);
void TCOMP_plot();

#endif //GPSDO_TCOMP_H
