/**
 * Interrupt Service Routines
 * @author Robert J. Rouquette
 * @date 2022-04-13
 */

#ifndef TM4C_INTERRUPTS_H
#define TM4C_INTERRUPTS_H

// System Exception Handlers
void Except_NMI(void);
void Except_SVC(void);
void Except_Debug(void);
void Except_PendSV(void);
void Except_SysTick(void);

// Fault Handlers
void Fault_Hard(void);
void Fault_Memory(void);
void Fault_Bus(void);
void Fault_Usage(void);

// Interrupt Service Prototypes
void ISR_FlashCtrl(void);
void ISR_FloatingPointException();
void ISR_Hibernation(void);
void ISR_SystemCtrl(void);
void ISR_Tamper(void);
void ISR_WatchDogTimer(void);
void ISR_UDMASoftware(void);
void ISR_UDMAError(void);
// GPIO Interrupts
void ISR_GPIOPortA(void);
void ISR_GPIOPortB(void);
void ISR_GPIOPortC(void);
void ISR_GPIOPortD(void);
void ISR_GPIOPortE(void);
void ISR_GPIOPortF(void);
void ISR_GPIOPortG(void);
void ISR_GPIOPortH(void);
void ISR_GPIOPortJ(void);
void ISR_GPIOPortK(void);
void ISR_GPIOPortL(void);
void ISR_GPIOPortM(void);
void ISR_GPIOPortN(void);
// GPIO Port P
void ISR_GPIOPortP0(void);
void ISR_GPIOPortP1(void);
void ISR_GPIOPortP2(void);
void ISR_GPIOPortP3(void);
void ISR_GPIOPortP4(void);
void ISR_GPIOPortP5(void);
void ISR_GPIOPortP6(void);
void ISR_GPIOPortP7(void);
// GPIO Port Q
void ISR_GPIOPortQ0(void);
void ISR_GPIOPortQ1(void);
void ISR_GPIOPortQ2(void);
void ISR_GPIOPortQ3(void);
void ISR_GPIOPortQ4(void);
void ISR_GPIOPortQ5(void);
void ISR_GPIOPortQ6(void);
void ISR_GPIOPortQ7(void);
// CAN Bus Interrupts
void ISR_CAN0(void);
void ISR_CAN1(void);
// Ethernet Interrupt
void ISR_EthernetMAC(void);
// USB Interrupt
void ISR_USB0(void);
// EPI Interrupt
void ISR_EPI0(void);
// UART Interrupts
void ISR_UART0(void);
void ISR_UART1(void);
void ISR_UART2(void);
void ISR_UART3(void);
void ISR_UART4(void);
void ISR_UART5(void);
void ISR_UART6(void);
void ISR_UART7(void);
// SSI Interrupts
void ISR_SSI0(void);
void ISR_SSI1(void);
void ISR_SSI2(void);
void ISR_SSI3(void);
// I2C Interupts
void ISR_I2C0(void);
void ISR_I2C1(void);
void ISR_I2C2(void);
void ISR_I2C3(void);
void ISR_I2C4(void);
void ISR_I2C5(void);
void ISR_I2C6(void);
void ISR_I2C7(void);
void ISR_I2C8(void);
void ISR_I2C9(void);
// PWM Interrupts
void ISR_PWM0Generator0(void);
void ISR_PWM0Generator1(void);
void ISR_PWM0Generator2(void);
void ISR_PWM0Generator3(void);
void ISR_PWM0Fault(void);
// Quadrature Encoder Interrupt
void ISR_QEI0(void);
// Analog Comparator Interrupts
void ISR_AnalogComparator0(void);
void ISR_AnalogComparator1(void);
void ISR_AnalogComparator2(void);
// ADC Interrupts
void ISR_ADC0Sequence0(void);
void ISR_ADC0Sequence1(void);
void ISR_ADC0Sequence2(void);
void ISR_ADC0Sequence3(void);
void ISR_ADC1Sequence0(void);
void ISR_ADC1Sequence1(void);
void ISR_ADC1Sequence2(void);
void ISR_ADC1Sequence3(void);
// Timer Interrupts
void ISR_Timer0A(void);
void ISR_Timer0B(void);
void ISR_Timer1A(void);
void ISR_Timer1B(void);
void ISR_Timer2A(void);
void ISR_Timer2B(void);
void ISR_Timer3A(void);
void ISR_Timer3B(void);
void ISR_Timer4A(void);
void ISR_Timer4B(void);
void ISR_Timer5A(void);
void ISR_Timer5B(void);
void ISR_Timer6A(void);
void ISR_Timer6B(void);
void ISR_Timer7A(void);
void ISR_Timer7B(void);

// enable ISR
int ISR_enable(void(*isr)(void));
// enable ISR
int ISR_disable(void(*isr)(void));
// set ISR priority
int ISR_priority(void(*isr)(void), uint8_t priority);


#endif //TM4C_INTERRUPTS_H
