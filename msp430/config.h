/*
System Configuration
*/

#ifndef MSP430_CONFIG
#define MSP430_CONFIG

#include <stdint.h>


struct SysConf {
    // PPS offset trimming
    int32_t ppsErrorOffset;
    // PID loop coefficients
    int32_t pidDeriv, pidProp, pidInteg;
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
