//
// Created by robert on 5/18/22.
//

#include <math.h>
#include <stdlib.h>

#include "epd.h"
#include "plot.h"


void PLOT_setLine(const int x1, const int y1, const int x2, const int y2, const uint8_t color) {
    int dx = x2 - x1;
    int dy = y2 - y1;

    if(abs(dx) > abs(dy)) {
        float slope = (float)dy / (float)dx;
        if(dx >= 0) {
            for (int i = 0; i <= dx; i++) {
                int j = (int) lroundf(slope * (float)i);
                EPD_setPixel(i + x1, j + y1, color);
            }
        }
        else {
            for (int i = 0; i >= dx; i--) {
                int j = (int) lroundf(slope * (float)i);
                EPD_setPixel(i + x1, j + y1, color);
            }
        }
    }
    else {
        float slope = (float)dx / (float)dy;
        if(dy >= 0) {
            for (int i = 0; i <= dy; i++) {
                int j = (int) lroundf(slope * (float)i);
                EPD_setPixel(j + x1, i + y1, color);
            }
        }
        else {
            for (int i = 0; i >= dy; i--) {
                int j = (int) lroundf(slope * (float)i);
                EPD_setPixel(j + x1, i + y1, color);
            }
        }
    }
}

void PLOT_setRect(int x1, int y1, int x2, int y2, const uint8_t color) {
    if(x1 > x2) {
        int temp = x1;
        x1 = x2;
        x2 = temp;
    }
    if(y1 > y2) {
        int temp = y1;
        y1 = y2;
        y2 = temp;
    }

    for(int y = y1; y <= y2; y++) {
        for(int x = x1; x <= x2; x++) {
            EPD_setPixel(x, y, color);
        }
    }
}
