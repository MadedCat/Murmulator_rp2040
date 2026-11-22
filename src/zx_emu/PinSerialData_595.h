#pragma once

#include <pico.h>
#include <hardware/pio.h>

#ifndef LATCH_595_PIN
#define LATCH_595_PIN (26)
#endif
#ifndef CLK_595_PIN
#define CLK_595_PIN (27)
#endif
#ifndef DATA_595_PIN
#define DATA_595_PIN (28)
#endif
/*
#ifndef CLK_AY_PIN1
#define CLK_AY_PIN1 (21)
#endif
#ifndef CLK_AY_PIN2
#define CLK_AY_PIN2 (29)
#endif
*/
#define TSPIN_MODE_OFF  (0)
#define TSPIN_MODE_GP21 (1)
#define TSPIN_MODE_GP29 (2)


void Init_PWM_175(uint8_t tspin_mode);
void Deinit_PWM_175();

void send_to_595(uint16_t data);