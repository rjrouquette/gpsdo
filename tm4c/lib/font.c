//
// Created by robert on 4/16/22.
//

#include "epd.h"
#include "font.h"


void FONT_drawText(
        int x, int y, const char *text, const volatile uint8_t *font,
        uint8_t foreground, uint8_t background
) {
    uint16_t charWidth = font[0];
    uint16_t charHeight = font[1];
    uint16_t stride = ((charWidth * charHeight) + 7) / 8;
    while(text[0] != 0) {
        const volatile uint8_t *glyph = font + 2 + (stride * (uint8_t)text[0]);
        ++text;
        uint16_t offset = 0;
        for(uint16_t i = 0; i < charHeight; i++) {
            for(uint16_t j = 0; j < charWidth; j++) {
                uint8_t color;
                if(glyph[offset >> 3u] & (1u << (offset & 7u)))
                    color = foreground;
                else
                    color = background;
                // set pixel
                EPD_setPixel(x + j, y + i, color);
                ++offset;
            }
        }
        x += charWidth;
    }
}
