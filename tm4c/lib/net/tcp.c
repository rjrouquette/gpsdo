//
// Created by robert on 5/20/22.
//

#include "tcp.h"

void TCP_process(uint8_t *frame, int flen) {
    // discard malformed frames
    if(flen < 62) return;
}
