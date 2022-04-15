


int main(void) {
    for(;;) {
        __asm volatile("nop");
    }
}

// ISR Test
void ISR_Timer0A() {
    __asm volatile("nop");
}
