//
// Created by robert on 5/20/23.
//

#include "common.hpp"
#include "../clk/util.h"
#include "../net/util.h"

// lut for PTP clock accuracy codes
const float lutClkAccuracy[17] = {
        25e-9f, 100e-9f, 250e-9f, 1e-6f,
        2.5e-6f, 10e-6f, 25e-6f, 100e-6f,
        250e-6f, 1e-3f, 2.5e-3f, 10e-3f,
        25e-6f, 100e-3f, 250e-3f, 1.0f,
        10.0f
};

// IEEE 802.1AS broadcast MAC address (01:80:C2:00:00:0E)
const uint8_t gPtpMac[6] = { 0x01, 0x80, 0xC2, 0x00, 0x00, 0x0E };


void toPtpTimestamp(uint64_t ts, PTP2_TIMESTAMP *tsPtp) {
    auto &scratch = reinterpret_cast<fixed_32_32&>(ts);
    // set seconds
    tsPtp->secondsHi = 0;
    tsPtp->secondsLo = htonl(scratch.ipart);
    // convert fraction to nanoseconds
    scratch.ipart = 0;
    scratch.full *= 1000000000u;
    tsPtp->nanoseconds = htonl(scratch.ipart);
}

uint64_t fromPtpTimestamp(PTP2_TIMESTAMP *tsPtp) {
    fixed_32_32 scratch;
    scratch.fpart = nanosToFrac(htonl(tsPtp->nanoseconds));
    scratch.ipart = htonl(tsPtp->secondsLo);
    return scratch.full;
}

uint32_t toPtpClkAccuracy(float rmsError) {
    // check accuracy thresholds
    for(int i = 0; i < 17; i++) {
        if(rmsError <= lutClkAccuracy[i])
            return 0x20 + i;
    }
    // greater than 10s
    return 0x31;
}
