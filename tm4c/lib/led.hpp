//
// Created by robert on 4/15/22.
//

#pragma once

void LED_init();

void LED0_ON();
void LED0_OFF();
void LED0_TGL();

void LED1_ON();
void LED1_OFF();
void LED1_TGL();

void LED_act0();
void LED_act1();

[[noreturn]]
void faultBlink(int a, int b);
