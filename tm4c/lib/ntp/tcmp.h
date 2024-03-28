//
// Created by robert on 4/30/23.
//

#ifndef GPSDO_TCMP_H
#define GPSDO_TCMP_H

/**
 * Initialize the temperature compensation module
 */
void TCMP_init();

/**
 * Get the current temperature measurement
 * @return the current temperature in Celsius
 */
float TCMP_temp();

/**
 * Get the current temperature correction value
 * @return frequency correction value
 */
float TCMP_get();

/**
 * Update the temperature compensation with a new sample
 * @param target new compensation value for current temperature
 * @param weight relative confidence in target accuracy
 */
void TCMP_update(float target, float weight);

/**
 * Write current status of the temperature compensation to a buffer
 * @param buffer destination for status information
 * @return number of bytes written to buffer
 */
unsigned TCMP_status(char *buffer);

#endif //GPSDO_TCMP_H
