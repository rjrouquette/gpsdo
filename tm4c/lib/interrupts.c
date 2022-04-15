//
// Created by robert on 4/13/22.
//

#include "interrupts.h"

// stack pointer
extern int _stack_ptr;
// .text/code, stored in Flash
extern int _etext;
// .data, copied into RAM on boot
extern int _data;
extern int _edata;
// .bss, unitialized variables
extern int _bss;
extern int _ebss;

// reset handler
void ISR_Reset(void);

#define RESERVED (0)

// ISR Vector Table
__attribute__((section(".vector_table")))
void * isr_table[130] = {
        // Stack Starting Address (0)
        &_stack_ptr,

        // Reset Vector (1)
        ISR_Reset,

        // Exception and Fault Vectors (2)
        Except_NMI,
        Fault_Hard,
        Fault_Memory,
        Fault_Bus,
        Fault_Usage,
        RESERVED,  // Reserved (7)
        RESERVED,  // Reserved (8)
        RESERVED,  // Reserved (9)
        RESERVED,  // Reserved (10)
        Except_SVC,
        Except_Debug,
        RESERVED,  // Reserved (13)
        Except_PendSV,
        Except_SysTick,

        // Interrupt Vectors (16)
        ISR_GPIOPortA,
        ISR_GPIOPortB,
        ISR_GPIOPortC,
        ISR_GPIOPortD,
        ISR_GPIOPortE,
        ISR_UART0,
        ISR_UART1,
        ISR_SPI0,
        ISR_I2C0,
        ISR_PWM0Fault,
        ISR_PWM0Generator0,
        ISR_PWM0Generator1,
        ISR_PWM0Generator2,
        ISR_QEI0,
        ISR_ADC0Sequence0,
        ISR_ADC0Sequence1,
        ISR_ADC0Sequence2,
        ISR_ADC0Sequence3,
        ISR_WatchDogTimer,
        ISR_Timer0A,
        ISR_Timer0B,
        ISR_Timer1A,
        ISR_Timer1B,
        ISR_Timer2A,
        ISR_Timer2B,
        ISR_AnalogComparator0,
        ISR_AnalogComparator1,
        ISR_AnalogComparator2,
        ISR_SystemCtrl,
        ISR_FlashCtrl,
        ISR_GPIOPortF,
        ISR_GPIOPortG,
        ISR_GPIOPortH,
        ISR_UART2,
        ISR_SPI1,
        ISR_Timer3A,
        ISR_Timer3B,
        ISR_I2C1,
        ISR_CAN0,
        ISR_CAN1,
        ISR_EthernetMAC,
        ISR_Hibernation,
        ISR_USB0,
        ISR_PWM0Generator3,
        ISR_UDMASoftware,
        ISR_UDMAError,
        ISR_ADC1Sequence0,
        ISR_ADC1Sequence1,
        ISR_ADC1Sequence2,
        ISR_ADC1Sequence3,
        ISR_EPI0,
        ISR_GPIOPortJ,
        ISR_GPIOPortK,
        ISR_GPIOPortL,
        ISR_SPI2,
        ISR_SPI3,
        ISR_UART3,
        ISR_UART4,
        ISR_UART5,
        ISR_UART6,
        ISR_UART7,
        ISR_I2C2,
        ISR_I2C3,
        ISR_Timer4A,
        ISR_Timer4B,
        ISR_Timer5A,
        ISR_Timer5B,
        ISR_FloatingPointException,
        RESERVED, // Reserved (84)
        RESERVED, // Reserved (85)
        ISR_I2C4,
        ISR_I2C5,
        ISR_GPIOPortM,
        ISR_GPIOPortN,
        RESERVED, // Reserved (90)
        ISR_Tamper,
        ISR_GPIOPortP0,
        ISR_GPIOPortP1,
        ISR_GPIOPortP2,
        ISR_GPIOPortP3,
        ISR_GPIOPortP4,
        ISR_GPIOPortP5,
        ISR_GPIOPortP6,
        ISR_GPIOPortP7,
        ISR_GPIOPortQ0,
        ISR_GPIOPortQ1,
        ISR_GPIOPortQ2,
        ISR_GPIOPortQ3,
        ISR_GPIOPortQ4,
        ISR_GPIOPortQ5,
        ISR_GPIOPortQ6,
        ISR_GPIOPortQ7,
        RESERVED, // Reserved (108)
        RESERVED, // Reserved (109)
        RESERVED, // Reserved (110)
        RESERVED, // Reserved (111)
        RESERVED, // Reserved (112)
        RESERVED, // Reserved (113)
        ISR_Timer6A,
        ISR_Timer6B,
        ISR_Timer7A,
        ISR_Timer7B,
        ISR_I2C6,
        ISR_I2C7,
        RESERVED, // Reserved (120)
        RESERVED, // Reserved (121)
        RESERVED, // Reserved (122)
        RESERVED, // Reserved (123)
        RESERVED, // Reserved (124)
        ISR_I2C8,
        ISR_I2C9,
        RESERVED, // Reserved (127)
        RESERVED, // Reserved (128)
        RESERVED, // Reserved (129)
};

//main() of your program
int main(void);
// reset handler
void ISR_Reset(void) {
    // load .data section into RAM
    int *src = &_etext;
    for (int *dest = &_data; dest < &_edata; dest++)
        *dest = *src++;

    // initialize .bss to zero
    for (int *dest = &_bss; dest < &_ebss; dest++)
        *dest = 0;

    // launch main application
    main();
}

// Default ISR does nothing
void ISR_Default(void) {
    asm("nop");
}

#define ISR_DEFAULT __attribute__((weak, alias("ISR_Default")))


