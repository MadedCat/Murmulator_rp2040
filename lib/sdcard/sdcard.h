#ifndef _SDCARD_H_
#define _SDCARD_H_

#include <pico.h>

#ifndef SDCARD_SPI_BUS
#define SDCARD_SPI_BUS          spi0
#endif

#ifndef SDCARD_SPI_SPI_BRG
#define SDCARD_SPI_SPI_BRG      8000//Khz
#endif

#ifndef SDCARD_PIN_SPI0_SCK
#define SDCARD_PIN_SPI0_SCK     2
#endif

#ifndef SDCARD_PIN_SPI0_MOSI
#define SDCARD_PIN_SPI0_MOSI    3
#endif

#ifndef SDCARD_PIN_SPI0_MISO
#define SDCARD_PIN_SPI0_MISO    4
#endif

#ifndef SDCARD_PIN_SPI0_CS
#define SDCARD_PIN_SPI0_CS      5
#endif

#endif // _SDCARD_H_
