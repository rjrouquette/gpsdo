//
// Created by robert on 5/20/23.
//

#include "../clk/util.h"
#include "common.h"

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
    union fixed_32_32 scratch;
    scratch.full = ts;
    // set seconds
    tsPtp->secondsHi = 0;
    tsPtp->secondsLo = __builtin_bswap32(scratch.ipart);
    // convert fraction to nanoseconds
    scratch.ipart = 0;
    scratch.full *= 1000000000u;
    tsPtp->nanoseconds = __builtin_bswap32(scratch.ipart);
}

uint64_t fromPtpTimestamp(PTP2_TIMESTAMP *tsPtp) {
    uint32_t nanos = __builtin_bswap32(tsPtp->nanoseconds);

    // multiply by the integer portion of 4.294967296
    nanos <<= 2;
    // apply correction factor for the fractional portion of 4.294967296
    union fixed_32_32 scratch;
    scratch.ipart = 0;
    scratch.fpart = nanos;
    scratch.full *= 0x12E0BE82;
    // round-to-nearest to minimize average error
    scratch.ipart += scratch.fpart >> 31;
    // combine correction value with base value
    scratch.fpart = nanos + scratch.ipart;
    // set integer seconds
    scratch.ipart = __builtin_bswap32(tsPtp->secondsLo);
    // return assembled result
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
