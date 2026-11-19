#pragma once

#include <pico.h>
#include <inttypes.h>
#include <stdbool.h>

#define PIO_VIDEO pio0
#define PIO_VIDEO_ADDR pio0

#ifndef beginVideo_PIN
#ifdef VGA_BASE_PIN
#define beginVideo_PIN (VGA_BASE_PIN)
#else
#define beginVideo_PIN (6)
#endif
#endif

typedef enum g_mode{
    g_mode_320x240x8bpp,
    g_mode_320x240x4bpp
}g_mode;

typedef enum g_out_TV{
    g_TV_OUT_PAL,
    g_TV_OUT_NTSC
}g_out_TV;


void graphics_init(g_out_TV g_out);
void graphics_set_buffer(uint8_t *buffer);
void graphics_set_hud_buffer(uint8_t *buffer);
void graphics_set_hud_handler(bool (*handler)(short));
void graphics_set_mode(g_mode mode);
void graphics_set_palette(uint8_t i, uint32_t color888);


//для совместимости
typedef enum fr_rate{
    rate_60Hz = 0,
    rate_72Hz = 1,
    rate_75Hz = 2,
    rate_85Hz = 3
}fr_rate;

typedef enum g_out{
    g_out_AUTO  = 0,
    g_out_NTSC  = 1,
    g_out_PAL   = 2
}g_out;

bool graphics_try_framerate(g_out g_out,fr_rate rate, bool apply);
