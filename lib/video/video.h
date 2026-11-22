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

#define HDMI_PIN_invert_diffpairs (1)
#define HDMI_PIN_RGB_notBGR (1)

#define beginHDMI_PIN_data (beginVideo_PIN+2)
#define beginHDMI_PIN_clk (beginVideo_PIN)

#define RGB888(r, g, b) ((r<<16) | (g << 8 ) | b )

#define TFT_MIN_BRIGHTNESS 20

typedef enum g_mode{
    g_mode_320x240x8bpp,
    g_mode_320x240x4bpp,
}g_mode;

typedef enum fr_rate{
    rate_60Hz = 0,
    rate_72Hz = 1,
    rate_75Hz = 2,
    rate_85Hz = 3
}fr_rate;

typedef enum rotate{
    rot_0Deg = 0,
    rot_90Deg = 1,
    rot_180Deg = 2,
    rot_360Deg = 3
}rotate;

typedef enum g_out{
    g_out_AUTO          = 0,
    g_out_VGA           = 1,
    g_out_HDMI          = 2,
    g_out_TFT_ST7789    = 3,
    g_out_TFT_ST7789V   = 4,
    g_out_TFT_ILI9341   = 5,
    g_out_TFT_ILI9341V  = 6,
    g_out_TFT_GC9A01    = 7    
}g_out;

g_out graphics_test_output();
void graphics_set_def_framerate(g_out v_out);
void graphics_init(g_out v_out,fr_rate rate);
void graphics_set_buffer(uint8_t *buffer);
void graphics_set_hud_buffer(uint8_t *buffer);
void graphics_set_hud_handler(bool (*handler)(short));
void graphics_set_mode(g_mode mode);
bool graphics_try_framerate(g_out v_out,fr_rate rate, bool apply);
void graphics_set_palette(uint8_t i, uint32_t color888);
void graphics_set_rotate(rotate degree);
void graphics_set_inversion(bool inversion);
void graphics_set_pixels(uint8_t pixels);
