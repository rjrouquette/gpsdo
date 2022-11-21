//
// Created by robert on 5/29/22.
//

#ifndef GPSDO_TCOMP_H
#define GPSDO_TCOMP_H

void TCOMP_updateTarget(float target);
void TCOMP_updateTemp(float temp);
float TCOMP_getComp(float *m, float *b);
void TCOMP_plot();

#endif //GPSDO_TCOMP_H
