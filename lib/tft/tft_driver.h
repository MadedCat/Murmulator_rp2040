#pragma once
#include <pico.h>
#include <hardware/pio.h>

#ifndef TFT_CS_PIN
#define TFT_CS_PIN (6)
#endif
#ifndef TFT_RST_PIN
#define TFT_RST_PIN (8)
#endif
#ifndef TFT_DC_PIN
#define TFT_DC_PIN (10)
#endif
#ifndef TFT_DATA_PIN
#define TFT_DATA_PIN (12)
#endif
#ifndef TFT_CLK_PIN
#define TFT_CLK_PIN (13)
#endif
#ifndef TFT_LED_PIN
#if TFT_ILI9341 || TFT_ST7789v
#define TFT_LED_PIN (9)
#endif
#endif

#if TFT_ILI9341 || TFT_ST7789v
	#define TFT_LED_PIN (9)
#endif

#ifdef TFT_ILI9341
	#define TFT_MIN_BRIGHTNESS 30
#endif

#ifdef TFT_ST7789v
	#define TFT_MIN_BRIGHTNESS 20
#endif

/*
#define TFT_CLK_PIN (10)//(17)
#define TFT_DATA_PIN (11)//(18)
#define TFT_RST_PIN (6)//(19)
#define TFT_DC_PIN (13)//(20)
#ifdef TFT_ST7789v
    #define TFT_CS_PIN (7) //(21)
#endif
*/


#define pio_SPI_TFT pio1
#define sm_SPI_TFT 0

#define pio_SPI_TFT_conv pio0
#define sm_SPI_TFT_conv 2


typedef enum g_mode{
    g_mode_320x240x8bpp,
    g_mode_320x240x4bpp
}g_mode;

typedef enum fr_rate{
    rate_60Hz = 0,
    rate_72Hz = 1,
    rate_75Hz = 2,
    rate_85Hz = 3
}fr_rate;

typedef enum g_out{
    g_out_AUTO          = 0,
    g_out_TFT_ST7789    = 3,
    g_out_TFT_ILI9341   = 4,
    g_out_TFT_ILI9341V  = 5,
    g_out_TFT_GC9A01    = 6,
    g_out_TFT_ST7789V   = 7
}g_out;

void graphics_init(g_out v_out,fr_rate rate);
void graphics_set_buffer(uint8_t *buffer);
void graphics_set_hud_buffer(uint8_t *buffer);
void graphics_set_hud_handler(bool (*handler)());
void graphics_set_mode(g_mode mode);
bool graphics_try_framerate(g_out v_out,fr_rate rate, bool apply);
void graphics_set_palette(uint8_t i, uint32_t color888);
