//
// Created by robert on 4/17/24.
//

#pragma once

#include <cstdint>

namespace clock::capture {
    /**
     * Get the filtered mean of the oscillator temperature sensor. (Kelvin)
     * @return the filtered mean of the oscillator temperature sensor (Kelvin)
     */
    float temperature();

    /**
     * Get the sampling noise of the oscillator temperature sensor. (Kelvin)
     * @return the sampling noise of the oscillator temperature sensor. (Kelvin)
     */
    float temperatureNoise();

    /**
     * Returns the raw offset between the ethernet PHY PPS output and the monotonic clock timer.
     * @return the raw offset of the ethernet PHY PPS output
     */
    uint32_t ppsEthernetRaw();

    /**
     * Returns the timestamp of the most recent GPS PPS edge capture in each clock domain
     * @param tsResult array of 3 that will be populated with
     *                 64-bit fixed-point format (32.32) timestamps <br/>
     *                 0 - monotonic clock <br/>
     *                 1 - compensated clock <br/>
     *                 2 - TAI clock
     */
    void ppsGps(uint64_t *tsResult);

    /**
     * Assemble 64-bit fixed-point timestamps from raw monotonic clock value
     * @param monoRaw raw monotonic clock value
     * @param stamps array of length 3 that will be populated with
     *               64-bit fixed-point format (32.32) timestamps <br/>
     *               0 - monotonic clock <br/>
     *               1 - compensated clock <br/>
     *               2 - TAI clock
     */
    void rawToFull(uint32_t monoRaw, uint64_t *stamps);
}
