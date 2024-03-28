//
// Created by robert on 4/15/22.
//

#ifndef GPSDO_LED_H
#define GPSDO_LED_H

#ifdef __cplusplus
extern "C" {
#else
#define static_assert _Static_assert
#endif

void LED_init();

void LED0_ON();
void LED0_OFF();
void LED0_TGL();

void LED1_ON();
void LED1_OFF();
void LED1_TGL();

void LED_act0();
void LED_act1();

_Noreturn
void faultBlink(int a, int b);

#ifdef __cplusplus
}
#endif

#endif //GPSDO_LED_H
