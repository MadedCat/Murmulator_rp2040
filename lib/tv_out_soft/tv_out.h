#pragma once

#include <pico.h>
#include <inttypes.h>
#include <stdbool.h>

#define PIO_VIDEO pio0

#ifndef beginVideo_PIN
#ifdef VGA_BASE_PIN
#define beginVideo_PIN (VGA_BASE_PIN)
#else
#define beginVideo_PIN (6)
#endif
#endif

typedef enum g_mode_t{
    g_mode_320x240x8bpp,
    g_mode_320x240x4bpp
}g_mode_t;

typedef enum g_out_TV_t{
    g_TV_OUT_PAL,
    g_TV_OUT_NTSC
}g_out_TV_t;


typedef enum NUM_TV_LINES_t
{    _624_lines,_625_lines,_524_lines,_525_lines,
}NUM_TV_LINES_t;

typedef enum COLOR_FREQ_t
{    _3579545,_4433619
}COLOR_FREQ_t;

typedef  struct tv_out_mode_t 
{
    // double color_freq;
    float  color_index;
    COLOR_FREQ_t c_freq;
    g_mode_t mode_bpp;
    g_out_TV_t tv_system;
    NUM_TV_LINES_t N_lines;
    bool cb_sync_PI_shift_lines;
    bool cb_sync_PI_shift_half_frame;
}tv_out_mode_t;

//для совместимости(удалить в других проектах)
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
bool graphics_try_framerate(g_out v_out,fr_rate rate, bool apply);

void graphics_init();
void graphics_set_buffer(uint8_t *buffer);
void graphics_set_hud_buffer(uint8_t *buffer);
void graphics_set_hud_handler(bool (*handler)(short));
void graphics_set_mode(tv_out_mode_t mode);
tv_out_mode_t graphics_get_default_mode(g_out video_out);
void graphics_set_palette(uint8_t i, uint32_t color888);



