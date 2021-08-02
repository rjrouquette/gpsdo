
#include "config.h"
#include "eeram.h"

struct SysConf sysConf;

/**
 * Fetch system configuration
**/
void SysConf_load() {
    EERAM_read(EERAM_CSA0, 0x0008u, &sysConf, sizeof(struct SysConf));
}

/**
 * Update non-volaltile configuration
**/
void SysConf_save() {
    EERAM_write(EERAM_CSA0, 0x0008u, &sysConf, sizeof(struct SysConf));
}
