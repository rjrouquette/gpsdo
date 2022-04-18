//
// Created by robert on 4/17/22.
//

#include "../hw/adc.h"
#include "temp.h"

void TEMP_init() {
    // Enable ADC0
    RCGCADC.EN_ADC0 = 1;

}

int16_t TEMP_get() {
    return 0;
}
