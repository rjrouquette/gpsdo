
#include "pps.h"


/**
 * Initialize the PPS module
**/
void PPS_init() {

}

/**
 * Reset the PPS ready indicator
**/
void PPS_clearReady() {

}

/**
 * Reset the PPS ready indicator
**/
uint8_t PPS_isReady() {
    return 0;
}

/**
 * Get the time delta between the generated PPS and GPS PPS leading edges.
 * Value is restricted to 16-bit range
 * LSB represents 5ns
 * @return the time offset between PPS leading edges
**/
int16_t PPS_getDelta() {
    return 0;
}
