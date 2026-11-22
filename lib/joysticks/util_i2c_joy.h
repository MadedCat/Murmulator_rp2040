#pragma once
#include <pico.h>
#include <inttypes.h>

#include "util_Wii_Joy.h"
#include "util_i2c_PCF857X.h"

#define QNT_IMP_NES (13)                // количество импульсов чтения джойстика NES

/*		PCF857X
adress		A0		A1		A2
			-		-		-	0x20		i2c_16_button
			+		-		-	0x21		i2c_NES_joy
			-		+		-	0x22		I2C_PCF_SEGA_JOY_ADDR		
			+		+		-	0x23		I2C_MCP_16BUTTON_ADDR
			-		-		+	0x24		I2C_MCP_NES_JOY_ADDR
			+		-		+	0X25		I2C_MCP_SEGA_JOY_ADDR
			-		+		+	0X26
			+		+		+	0x27
*/
#define I2C_PCF_16BUTTON_ADDR (0x20)
#define I2C_PCF_NES_JOY_ADDR (0x21)
#define I2C_PCF_SEGA_JOY_ADDR (0x22)
#define I2C_MCP_16BUTTON_ADDR (0x23)
#define I2C_MCP_NES_JOY_ADDR (0x24)
#define I2C_MCP_SEGA_JOY_ADDR (0x25)
#define I2C_PCF8574_8BUTTON_ADDR (0x26)

#ifndef i2c_joy_port
#define i2c_joy_port (i2c1)
#endif
#define CLOCK_I2C_100kHz (100)
#define CLOCK_I2C_400kHz (400)

#ifndef PICO_I2C_JOY_SDA_PIN
#define PICO_I2C_JOY_SDA_PIN (14)
#endif

#ifndef PICO_I2C_JOY_SCL_PIN
#define PICO_I2C_JOY_SCL_PIN (15)
#endif

extern uint8_t i2c_joy_data[16];

bool i2c_joy_start();
