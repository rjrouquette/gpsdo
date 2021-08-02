
#include "config.h"
#include "eeram.h"

struct SysConfig sysConfig;

/**
 * Fetch system configuration
**/
void SysConfig_load() {
    EERAM_read(EERAM_CSA0, 0x0008u, &sysConfig, sizeof(struct SysConfig));
}

/**
 * Update non-volaltile configuration
**/
void SysConfig_save() {
    EERAM_write(EERAM_CSA0, 0x0008u, &sysConfig, sizeof(struct SysConfig));
}
