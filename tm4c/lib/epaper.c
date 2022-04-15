/**
 * Driver Library for Waveshare 2.7 inch e-Paper HAT
 * @author Robert J. Rouquette
 * @date 2022-04-15
 */

#include "../hw/ssi.h"
#include "../lib/delay.h"
#include "epaper.h"

void EPAPER_init() {
    // configure SSI3 interface as SPI Master
    // enable SSI3 module
    RCGCSSI.EN_SSI3 = 1;

    // enable GPIO port Q
    RCGCGPIO.EN_PORTQ = 1;
    delay_cycles_4();

    // configure pins 0-3
    PORTQ.LOCK = GPIO_LOCK_KEY;
    PORTQ.CR = 0x0fu;
    PORTQ.AMSEL = 0x0fu;
    PORTQ.PCTL = 0xEEEE;
    PORTQ.AFSEL = 0x00u;
    PORTQ.DR2R = 0x0fu;
    PORTQ.DEN = 0x0fu;

    // lock GPIO config
    PORTQ.CR = 0;
    PORTQ.LOCK = 0;

    // configure SSI3
    SSI3.CR1.raw = 0;
    SSI3.CR0.raw = 0;
    // standard SPI mode
    SSI3.CR0.SPO = 0;
    SSI3.CR0.SPO = 1;
    // 8-bit data
    SSI3.CR0.DSS = SSI_DSS_8_BIT;
    // 2x prescaler
    SSI3.CPSR.CPSDVSR = 2;
    // use system clock
    SSI3.CC.CS = SSI_CLK_SYS;

}
