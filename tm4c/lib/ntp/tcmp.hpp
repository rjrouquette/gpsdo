//
// Created by robert on 4/30/23.
//

#pragma once

namespace tcmp {
    /**
     * Initialize the temperature compensation module
     */
    void init();

    /**
     * Get the current temperature measurement
     * @return the current temperature in Celsius
     */
    float temp();

    /**
     * Get the current temperature correction value
     * @return frequency correction value
     */
    float get();

    /**
     * Update the temperature compensation with a new sample
     * @param target new compensation value for current temperature
     * @param weight relative confidence in target accuracy
     */
    void update(float target, float weight);

    /**
     * Write current status of the temperature compensation to a buffer
     * @param buffer destination for status information
     * @return number of bytes written to buffer
     */
    unsigned status(char *buffer);
}
