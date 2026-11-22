#include <string.h>
#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"

#include "Joystics.h"
#include "util_i2c_joy.h"
#include "util_i2c_PCF857X.h"
#include "util_i2c_MCP23017.h"

uint8_t i2c_joy_data[16];

void i2c_joy_init(int CLOCK_I2C){
	i2c_init(i2c_joy_port, CLOCK_I2C*1000);
	gpio_set_function(PICO_I2C_JOY_SDA_PIN, GPIO_FUNC_I2C);
	gpio_set_function(PICO_I2C_JOY_SCL_PIN, GPIO_FUNC_I2C);
	gpio_pull_up(PICO_I2C_JOY_SDA_PIN);
	gpio_pull_up(PICO_I2C_JOY_SCL_PIN);
};

void i2c_joy_deinit(){
	i2c_deinit(i2c_joy_port);
	gpio_set_function(PICO_I2C_JOY_SDA_PIN, GPIO_FUNC_NULL);
	gpio_set_function(PICO_I2C_JOY_SCL_PIN, GPIO_FUNC_NULL);
	gpio_disable_pulls(PICO_I2C_JOY_SDA_PIN);
	gpio_disable_pulls(PICO_I2C_JOY_SCL_PIN);  
};


bool i2c_joy_start(){

	bool is_present=false;
	i2c_joy_init(CLOCK_I2C_100kHz);
	
	if(Init_Wii_Joystick()){Joystics.Present_WII_joy = true; is_present=true; printf ("WII joystick is present\n");} else {Joystics.Present_WII_joy = false;}

	if(!is_present){
		if (init_PCF857X(I2C_PCF8574_8BUTTON_ADDR)) {Joystics.Present_i2c_PCF_8_buttons = true; is_present=true; printf ("PCF8574 8 button  is present\n");} else {Joystics.Present_i2c_PCF_8_buttons = false;}
	}

	if(!is_present){
		i2c_joy_deinit();
		i2c_joy_init(CLOCK_I2C_400kHz);
		if (init_PCF857X(I2C_PCF_16BUTTON_ADDR)) {Joystics.Present_i2c_PCF_16_buttons = true; is_present=true; printf ("PCF8575 16 button is present\n");} else {Joystics.Present_i2c_PCF_16_buttons = false;}
		if (init_PCF857X(I2C_PCF_NES_JOY_ADDR)) {is_present=true; printf ("PCF8575 for NES is present\n");Joystics.Present_i2c_PCF_Joysticks=true;} else{Joystics.Present_i2c_PCF_Joysticks=false;}
		if (init_PCF857X(I2C_PCF_SEGA_JOY_ADDR)) {is_present=true; printf ("PCF8575 for SEGA is present\n");Joystics.Present_i2c_PCF_SEGA_joy=true;} else{Joystics.Present_i2c_PCF_SEGA_joy=false;}
		if (init_MCP23017(I2C_MCP_16BUTTON_ADDR)) {is_present=true; printf ("MCP23017 for 16 buttons is present\n");Joystics.Present_i2c_MCP_16_buttons=true;} else{Joystics.Present_i2c_MCP_16_buttons=false;}
		if (init_MCP23017(I2C_MCP_NES_JOY_ADDR)) {is_present=true;printf ("MCP23017 for NES joy is present\n");Joystics.Present_i2c_MCP_Joysticks = true;}else{Joystics.Present_i2c_MCP_Joysticks = false;}
		if (init_MCP23017(I2C_MCP_SEGA_JOY_ADDR)) {is_present=true;printf ("MCP23017 for SEGA joy is present\n");Joystics.Present_i2c_MCP_SEGA_joy = true;}else{Joystics.Present_i2c_MCP_SEGA_joy = false;}
		if(!is_present){i2c_joy_deinit();}
	}

	return is_present;
};