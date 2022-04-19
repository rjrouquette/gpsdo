/**
 * Interrupt Service Routines
 * @author Robert J. Rouquette
 * @date 2022-04-13
 */

#include <stdint.h>
#include "interrupts.h"

// stack pointer
extern int _stack_ptr;
// .text/code, stored in Flash
extern int _etext;
// .data, copied into RAM on boot
extern int _data;
extern int _edata;
// .bss, uninitialized variables
extern int _bss;
extern int _ebss;

// reset handler
void ISR_Reset(void);

#define RESERVED (0)

// ISR Vector Table
#define ISR_VECTOR_COUNT (130)
__attribute__((section(".isr_vector")))
void * volatile const isr_table[ISR_VECTOR_COUNT] = {
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
        ISR_SSI0,
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
        ISR_SSI1,
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
        ISR_SSI2,
        ISR_SSI3,
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

// Default ISR does nothing
void ISR_Default(void) {
    __asm volatile("nop");
}

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

    // enable registered interrupts
    for(int i = 16; i < ISR_VECTOR_COUNT; i++) {
        if(isr_table[i] == 0) continue;
        if(isr_table[i] == ISR_Default) continue;
        int id = i - 16;
        ((uint32_t *) 0xE000E100)[(id >> 5u) & 0x3u] |= (1u << (id & 0x1Fu));
    }

    // launch main application
    main();
}

#define ISR_DEFAULT __attribute__((weak, alias("ISR_Default")))

// System Exception Handlers
void Except_NMI(void) ISR_DEFAULT;
void Except_SVC(void) ISR_DEFAULT;
void Except_Debug(void) ISR_DEFAULT;
void Except_PendSV(void) ISR_DEFAULT;
void Except_SysTick(void) ISR_DEFAULT;

// Fault Handlers
void Fault_Hard(void) ISR_DEFAULT;
void Fault_Memory(void) ISR_DEFAULT;
void Fault_Bus(void) ISR_DEFAULT;
void Fault_Usage(void) ISR_DEFAULT;

// Interrupt Service Prototypes
void ISR_FlashCtrl(void) ISR_DEFAULT;
void ISR_FloatingPointException() ISR_DEFAULT;
void ISR_Hibernation(void) ISR_DEFAULT;
void ISR_SystemCtrl(void) ISR_DEFAULT;
void ISR_Tamper(void) ISR_DEFAULT;
void ISR_WatchDogTimer(void) ISR_DEFAULT;
void ISR_UDMASoftware(void) ISR_DEFAULT;
void ISR_UDMAError(void) ISR_DEFAULT;
// GPIO Interrupts
void ISR_GPIOPortA(void) ISR_DEFAULT;
void ISR_GPIOPortB(void) ISR_DEFAULT;
void ISR_GPIOPortC(void) ISR_DEFAULT;
void ISR_GPIOPortD(void) ISR_DEFAULT;
void ISR_GPIOPortE(void) ISR_DEFAULT;
void ISR_GPIOPortF(void) ISR_DEFAULT;
void ISR_GPIOPortG(void) ISR_DEFAULT;
void ISR_GPIOPortH(void) ISR_DEFAULT;
void ISR_GPIOPortJ(void) ISR_DEFAULT;
void ISR_GPIOPortK(void) ISR_DEFAULT;
void ISR_GPIOPortL(void) ISR_DEFAULT;
void ISR_GPIOPortM(void) ISR_DEFAULT;
void ISR_GPIOPortN(void) ISR_DEFAULT;
// GPIO Port P
void ISR_GPIOPortP0(void) ISR_DEFAULT;
void ISR_GPIOPortP1(void) ISR_DEFAULT;
void ISR_GPIOPortP2(void) ISR_DEFAULT;
void ISR_GPIOPortP3(void) ISR_DEFAULT;
void ISR_GPIOPortP4(void) ISR_DEFAULT;
void ISR_GPIOPortP5(void) ISR_DEFAULT;
void ISR_GPIOPortP6(void) ISR_DEFAULT;
void ISR_GPIOPortP7(void) ISR_DEFAULT;
// GPIO Port Q
void ISR_GPIOPortQ0(void) ISR_DEFAULT;
void ISR_GPIOPortQ1(void) ISR_DEFAULT;
void ISR_GPIOPortQ2(void) ISR_DEFAULT;
void ISR_GPIOPortQ3(void) ISR_DEFAULT;
void ISR_GPIOPortQ4(void) ISR_DEFAULT;
void ISR_GPIOPortQ5(void) ISR_DEFAULT;
void ISR_GPIOPortQ6(void) ISR_DEFAULT;
void ISR_GPIOPortQ7(void) ISR_DEFAULT;
// CAN Bus Interrupts
void ISR_CAN0(void) ISR_DEFAULT;
void ISR_CAN1(void) ISR_DEFAULT;
// Ethernet Interrupt
void ISR_EthernetMAC(void) ISR_DEFAULT;
// USB Interrupt
void ISR_USB0(void) ISR_DEFAULT;
// EPI Interrupt
void ISR_EPI0(void) ISR_DEFAULT;
// UART Interrupts
void ISR_UART0(void) ISR_DEFAULT;
void ISR_UART1(void) ISR_DEFAULT;
void ISR_UART2(void) ISR_DEFAULT;
void ISR_UART3(void) ISR_DEFAULT;
void ISR_UART4(void) ISR_DEFAULT;
void ISR_UART5(void) ISR_DEFAULT;
void ISR_UART6(void) ISR_DEFAULT;
void ISR_UART7(void) ISR_DEFAULT;
// SSI Interrupts
void ISR_SSI0(void) ISR_DEFAULT;
void ISR_SSI1(void) ISR_DEFAULT;
void ISR_SSI2(void) ISR_DEFAULT;
void ISR_SSI3(void) ISR_DEFAULT;
// I2C Interupts
void ISR_I2C0(void) ISR_DEFAULT;
void ISR_I2C1(void) ISR_DEFAULT;
void ISR_I2C2(void) ISR_DEFAULT;
void ISR_I2C3(void) ISR_DEFAULT;
void ISR_I2C4(void) ISR_DEFAULT;
void ISR_I2C5(void) ISR_DEFAULT;
void ISR_I2C6(void) ISR_DEFAULT;
void ISR_I2C7(void) ISR_DEFAULT;
void ISR_I2C8(void) ISR_DEFAULT;
void ISR_I2C9(void) ISR_DEFAULT;
// PWM Interrupts
void ISR_PWM0Generator0(void) ISR_DEFAULT;
void ISR_PWM0Generator1(void) ISR_DEFAULT;
void ISR_PWM0Generator2(void) ISR_DEFAULT;
void ISR_PWM0Generator3(void) ISR_DEFAULT;
void ISR_PWM0Fault(void) ISR_DEFAULT;
// Quadrature Encoder Interrupt
void ISR_QEI0(void) ISR_DEFAULT;
// Analog Comparator Interrupts
void ISR_AnalogComparator0(void) ISR_DEFAULT;
void ISR_AnalogComparator1(void) ISR_DEFAULT;
void ISR_AnalogComparator2(void) ISR_DEFAULT;
// ADC Interrupts
void ISR_ADC0Sequence0(void) ISR_DEFAULT;
void ISR_ADC0Sequence1(void) ISR_DEFAULT;
void ISR_ADC0Sequence2(void) ISR_DEFAULT;
void ISR_ADC0Sequence3(void) ISR_DEFAULT;
void ISR_ADC1Sequence0(void) ISR_DEFAULT;
void ISR_ADC1Sequence1(void) ISR_DEFAULT;
void ISR_ADC1Sequence2(void) ISR_DEFAULT;
void ISR_ADC1Sequence3(void) ISR_DEFAULT;
// Timer Interrupts
void ISR_Timer0A(void) ISR_DEFAULT;
void ISR_Timer0B(void) ISR_DEFAULT;
void ISR_Timer1A(void) ISR_DEFAULT;
void ISR_Timer1B(void) ISR_DEFAULT;
void ISR_Timer2A(void) ISR_DEFAULT;
void ISR_Timer2B(void) ISR_DEFAULT;
void ISR_Timer3A(void) ISR_DEFAULT;
void ISR_Timer3B(void) ISR_DEFAULT;
void ISR_Timer4A(void) ISR_DEFAULT;
void ISR_Timer4B(void) ISR_DEFAULT;
void ISR_Timer5A(void) ISR_DEFAULT;
void ISR_Timer5B(void) ISR_DEFAULT;
void ISR_Timer6A(void) ISR_DEFAULT;
void ISR_Timer6B(void) ISR_DEFAULT;
void ISR_Timer7A(void) ISR_DEFAULT;
void ISR_Timer7B(void) ISR_DEFAULT;

static int id_isr(void(*isr)(void)) {
    // verify that ISR is implemented
    if(isr == 0) return -1;
    if(isr == ISR_Default) return -1;
    // locate ISR in vector table
    for(int i = 0; i < ISR_VECTOR_COUNT; i++) {
        if(isr_table[i] == isr) return i;
    }
    // ISR is not mapped in vector table
    return -1;
}

// enable ISR
int ISR_enable(void(*isr)(void)) {
    int id = id_isr(isr);
    // invalid ISR address
    if(id < 0) return id;
    // ISR is Fault handler
    if(id < 16) return id;
    // ISR is mapped and valid
    int offset = id - 16;
    ((uint32_t *) 0xE000E100)[(offset >> 5u) & 0x3u] |= (1u << (offset & 0x1Fu));
    return id;
}

// disable ISR
int ISR_disable(void(*isr)(void)) {
    int id = id_isr(isr);
    // invalid ISR address
    if(id < 0) return id;
    // ISR is Fault handler
    if(id < 16) return id;
    // ISR is mapped and valid
    int offset = id - 16;
    ((uint32_t *) 0xE000E180)[(offset >> 5u) & 0x3u] |= (1u << (offset & 0x1Fu));
    return id;
}

// set ISR priority
int ISR_priority(void(*isr)(void), uint8_t priority) {
    int id = id_isr(isr);
    // invalid ISR address
    if(id < 0) return id;
    // ISR is Fault handler
    if(id < 16) return id;
    // ISR is mapped and valid
    int offset = id - 16;
    uint32_t reg_pri = ((uint32_t *) 0xE000E400)[(offset >> 2u) & 0x1Fu];
    reg_pri &= ~(0xF0u << (8u * (offset & 0x3u)));
    reg_pri |= (0xF0u & (((uint32_t) priority) << 5u)) << (8u * (offset & 0x3u));
    ((uint32_t *) 0xE000E400)[(offset >> 2u) & 0x1Fu] = reg_pri;
    return id;
}
