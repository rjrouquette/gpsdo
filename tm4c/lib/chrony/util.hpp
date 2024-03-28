//
// Created by robert on 3/27/24.
//

#pragma once

#include "candm.h"

namespace chrony {
    /**
     * convert IEEE 754 to candm float format
     * @param value IEEE 754 single-precision float
     * @return candm float format
     */
    int32_t htonf(float value);

    void toTimespec(uint64_t timestamp, volatile Timespec *ts);
}
