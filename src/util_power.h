#pragma once
#include "inttypes.h"
#include <stdlib.h>
#include <pico.h>
#include "time.h"
#include "inttypes.h"
#include "stdbool.h"

//MAX17043

#ifndef _POWER_MON_H
#define _POWER_MON_H

#define POWER_PORT (i2c1)
#define POWER_CLOCK (400000)
#ifndef POWER_SDA_PIN
#define POWER_SDA_PIN (14)
#define POWER_SCL_PIN (15)
#endif

#define POWER_BYTE_COUNT 4

#define POWER_ADDRESS	0x36

#define VCELL_REGISTER		0x02
#define SOC_REGISTER		0x04
#define MODE_REGISTER		0x06
#define VERSION_REGISTER	0x08
#define CONFIG_REGISTER		0x0C
#define COMMAND_REGISTER	0xFE

bool power_init();

uint16_t power_getVCell();
uint8_t power_getSoC();
int power_getVersion();
uint8_t power_getCompensateValue();
uint8_t power_getAlertThreshold();
void power_setAlertThreshold(uint8_t threshold);
bool power_inAlert();
void power_clearAlert();
		
bool power_reset();
bool power_quickStart();

bool power_readConfigRegister();
bool power_readRegister(uint8_t startAddress);
bool power_writeRegister(uint8_t address,uint8_t MSB,uint8_t LSB);

extern uint8_t battery_power_percent;
extern bool battery_power_charge;

//bool timer_init_battery_check();
//static bool timer_callback_adc(repeating_timer_t *rt);
void monitor_battery_voltage();
bool batt_management_init();
void batt_management_stop();
bool batt_management_usb_power_detected();


#endif