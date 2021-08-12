/*
System Configuration
*/

#ifndef MSP430_CONFIG
#define MSP430_CONFIG

#include <stdint.h>


struct SysConf {
    // PID loop coefficients
    struct {
        int32_t P, I, D;
    } pid;
    // PPS offset trimming
    struct {
        int16_t trim;
    } pps;
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
