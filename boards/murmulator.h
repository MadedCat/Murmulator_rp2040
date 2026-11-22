#pragma once
#if PICO_RP2350
#include "boards/pico2.h"

#else
#include "boards/pico.h"
#endif

// SDCARD
#define SDCARD_PIN_SPI0_CS 5
#define SDCARD_PIN_SPI0_SCK 2
#define SDCARD_PIN_SPI0_MOSI 3
#define SDCARD_PIN_SPI0_MISO 4

// PS2KBD
#define PS2KBD_GPIO_FIRST 0

// NES Gamepad
#define D_JOY_CLK_PIN 14
#define D_JOY_LATCH_PIN 15
#define D_JOY_DATA_PIN 16
#define D_JOY1_DATA_PIN 16
#define D_JOY2_DATA_PIN 17

#define POWER_SDA_PIN (14)
#define POWER_SCL_PIN (15)

// VGA 8 pins starts from pin:
#define VGA_BASE_PIN 6

// HDMI 8 pins starts from pin:
#define HDMI_BASE_PIN 6

// TFT
#define TFT_CS_PIN 6
#define TFT_RST_PIN 8
#define TFT_LED_PIN 9
#define TFT_DC_PIN 10
#define TFT_DATA_PIN 12
#define TFT_CLK_PIN 13

#define SMS_SINGLE_FILE 1

// 74HC595 / AY
#define LATCH_595_PIN 26
#define CLK_595_PIN 27
#define DATA_595_PIN 28
#define CLK_AY_PIN1 21
#define CLK_AY_PIN2 29

// Misc
#define PIN_ZX_LOAD 22
#define ZX_AY_PWM_PIN0 26
#define ZX_AY_PWM_PIN1 27
#define ZX_BEEP_PIN 28
#define WORK_LED_PIN PICO_DEFAULT_LED_PIN

// Sound
#if defined(AUDIO_PWM)
#define AUDIO_PWM_PIN 26
/// TODO: remove it
#define AUDIO_DATA_PIN 26
#define AUDIO_CLOCK_PIN 27
#else
// I2S Sound
#define AUDIO_DATA_PIN 26
#define AUDIO_CLOCK_PIN 27
#endif