/*
System Configuration
*/

#ifndef MSP430_CONFIG
#define MSP430_CONFIG

#include <stdint.h>


struct SysConf {
    // PID loop coefficients
    int32_t pidDeriv, pidProp, pidInteg;
    // PPS offset trimming
    int16_t ppsOffset;
};

// global system configuration
extern struct SysConf sysConf;

/**
 * Fetch system configuration
**/
void SysConf_load();

/**
 * Update non-volaltile configuration
**/
void SysConf_save();

#endif
