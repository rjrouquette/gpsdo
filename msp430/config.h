/*
System Configuration
*/

#ifndef MSP430_CONFIG
#define MSP430_CONFIG

#include <stdint.h>


struct SysConfig {
    // PPS offset trimming
    int32_t ppsErrorOffset;
    // PID loop coefficients
    int32_t pidDeriv, pidProp, pidInteg;
};

// global system configuration
extern struct SysConfig sysConfig;

/**
 * Fetch system configuration
**/
void SysConfig_load();

/**
 * Update non-volaltile configuration
**/
void SysConfig_save();

#endif
