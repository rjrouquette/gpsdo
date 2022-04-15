


int main(void) {
    for(;;) {
        __asm volatile("mov r0, r0");
    }
}

// ISR Test
void ISR_Timer0A() {
    __asm volatile("mov r0, r0");
}
