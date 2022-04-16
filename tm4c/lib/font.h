//
// Created by robert on 4/16/22.
//

#ifndef GPSDO_FONT_H
#define GPSDO_FONT_H

#include <stdint.h>

extern const volatile uint8_t FONT_ASCII_16[];

typedef void (*SetPixel)(int x, int y, uint8_t color);

void FONT_drawText(
        int x, int y, char *text, const volatile uint8_t *font,
        uint8_t foreground, uint8_t background, SetPixel callback
);


#endif //GPSDO_FONT_H
