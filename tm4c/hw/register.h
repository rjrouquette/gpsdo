//
// Created by robert on 4/26/22.
//

#ifndef GPSDO_REGISTER_H
#define GPSDO_REGISTER_H

#include <stdint.h>

#define REGMAP_32(type, fields) union type { struct fields; uint32_t raw; }

#define PAGE_MAP(type, fields) struct type fields; _Static_assert(sizeof(struct type) == 4096, "PERIPHERAL_MAP must be 4096 bytes");

#endif //GPSDO_REGISTER_H
