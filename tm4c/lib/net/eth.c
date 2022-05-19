//
// Created by robert on 5/16/22.
//

#include "eth.h"

int ETH_isARP(const uint8_t *ethType) {
    return ethType[0] == 0x08 && ethType[1] == 0x06;
}

int ETH_isIPv4(const uint8_t *ethType) {
    return ethType[0] == 0x08 && ethType[1] == 0x00;
}
