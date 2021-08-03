
#include "config.h"
#include "eeram.h"

struct SysConf sysConf;

/**
 * Fetch system configuration
**/
void SysConf_load() {
    EERAM_read(8u, &sysConf, sizeof(struct SysConf));
}

/**
 * Update non-volaltile configuration
**/
void SysConf_save() {
    EERAM_write(8u, &sysConf, sizeof(struct SysConf));
}
