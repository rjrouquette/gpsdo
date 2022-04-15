
#include "lib/gpio.h"
#include "lib/sys.h"

void delay() {
    volatile int delay = 1u<<20u;
    while(delay)
        --delay;
}

int main(void) {
    RCGCGPIO.EN_PORTN = 1;
    __asm volatile("nop");
    __asm volatile("nop");
    __asm volatile("nop");
    __asm volatile("nop");

    PORTN.LOCK = 0x4C4F434B;
    PORTN.CR = 0x01u;
    PORTN.AMSEL = 0;
    PORTN.PCTL = 0;
    PORTN.AFSEL = 0;
    PORTN.DR8R = 0x01u;
    PORTN.DIR = 0x01u;
    PORTN.DEN = 0x01u;
    PORTN.DATA[0x01u] = 0x01u;
    for(;;) {
        delay();
        PORTN.DATA[0x01u] ^= 0x01u;
    }
}

// ISR Test
void ISR_Timer0A() {
    __asm volatile("nop");
}
