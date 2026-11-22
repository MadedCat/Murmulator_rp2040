#pragma once
#include <inttypes.h>
#include <pico.h>

#ifndef D_JOY1_DATA_PIN
#ifdef D_JOY1_DATA_PIN
#define D_JOY1_DATA_PIN (D_JOY1_DATA_PIN)
#elif defined(D_JOY_DATA_PIN)
#define D_JOY1_DATA_PIN (D_JOY_DATA_PIN)
#else
#define D_JOY1_DATA_PIN (16)
#endif
#endif

#ifndef D_JOY2_DATA_PIN
#ifdef D_JOY2_DATA_PIN
#define D_JOY2_DATA_PIN (D_JOY2_DATA_PIN)
#elif defined(D_JOY_DATA_PIN)
#define D_JOY2_DATA_PIN (D_JOY_DATA_PIN)
#else
#define D_JOY2_DATA_PIN (17)
#endif
#endif

#ifndef D_JOY_CLK_PIN
#ifdef D_JOY_CLK_PIN
#define D_JOY_CLK_PIN (D_JOY_CLK_PIN)
#else
#define D_JOY_CLK_PIN (14)
#endif
#endif

#ifndef D_JOY_LATCH_PIN
#ifdef D_JOY_LATCH_PIN
#define D_JOY_LATCH_PIN (D_JOY_LATCH_PIN)
#else
#define D_JOY_LATCH_PIN (15)
#endif
#endif

#define QNT_IMP_NES (13)                // количество импульсов чтения джойстика NES

uint32_t d_joy_get_data();
bool decode_joy();
void d_joy_init();