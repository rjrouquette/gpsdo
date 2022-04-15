
#include "lib/gpio.h"

void delay() {
    volatile int delay = 1u<<20u;
    while(delay)
        --delay;
}

int main(void) {
    *(uint32_t*)(0x400FE608) |= (1u << 12u);
    __asm volatile("nop");
    __asm volatile("nop");
    __asm volatile("nop");
    __asm volatile("nop");

    GPION.LOCK = 0x4C4F434B;
    GPION.CR = 0x01u;
    GPION.AMSEL = 0;
    GPION.PCTL = 0;
    GPION.AFSEL = 0;
    GPION.DR8R = 0x01u;
    GPION.DIR = 0x01u;
    GPION.DEN = 0x01u;
    GPION.DATA[0x01u] = 0x01u;
    for(;;) {
        delay();
        GPION.DATA[0x01u] ^= 0x01u;
    }
}

// ISR Test
void ISR_Timer0A() {
    __asm volatile("nop");
}
