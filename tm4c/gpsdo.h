//
// Created by robert on 5/24/22.
//

#ifndef GPSDO_GPSDO_H
#define GPSDO_GPSDO_H

void GPSDO_init();
void GPSDO_run();
int GPSDO_isLocked();
int GPSDO_offsetNano();
float GPSDO_offsetMean();
float GPSDO_offsetRms();
float GPSDO_skewRms();
float GPSDO_freqCorr();
float GPSDO_temperature();
float GPSDO_compBias();
float GPSDO_compOffset();
float GPSDO_compCoeff();
float GPSDO_compValue();
float GPSDO_pllTrim();

#endif //GPSDO_GPSDO_H
