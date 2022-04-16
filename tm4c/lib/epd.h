/**
 * Driver Library for Waveshare 2.7 inch e-Paper HAT
 * @author Robert J. Rouquette
 * @date 2022-04-15
 */

#ifndef GPSDO_EPD_H
#define GPSDO_EPD_H

void EPD_init();
int EPG_width();
int EPG_height();
void EPD_clear();
void EPD_refresh();
void EPD_setPixel(int x, int y, uint8_t color);

#endif //GPSDO_EPD_H
