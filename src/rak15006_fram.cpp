/**
 * @file rak15006_fram.cpp
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief Initialization, read and write functions for FRAM
 * @version 0.1
 * @date 2023-08-02
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "main.h"
#include <Preferences.h>

#define FRAM_WP_PIN WB_IO1
#define FRAM_4M_SIZE 0x80000

/** ESP32 preferences */
Preferences power_prefs;

bool init_rak15006(void)
{
	power_prefs.begin("Power", false);

	bool hasPref = power_prefs.getBool("valid", false);
	if (!hasPref)
	{
		power_prefs.begin("Power", false);
		power_prefs.putBool("valid", true);
		power_prefs.putLong("m_e", 0);
		power_prefs.putLong("y_e", 0);
	}

	power_prefs.end();
	return true;
}

void read_rak15006(uint32_t memaddr, uint8_t *obj, uint16_t size)
{
	bool valid_addr = false;
	power_prefs.begin("Power", true);
	long_byte_s fram_val;

	if (memaddr == g_month_energy_addr)
	{
		fram_val.val_32 = power_prefs.getLong("m_e", 0);
		valid_addr = true;
	}
	if (memaddr == g_year_energy_addr)
	{
		fram_val.val_32 = power_prefs.putLong("y_e", 0);
		valid_addr = true;
	}
	power_prefs.end();

	if (valid_addr)
	{
		obj[0] = fram_val.val_8[0];
		obj[1] = fram_val.val_8[1];
		obj[2] = fram_val.val_8[2];
		obj[3] = fram_val.val_8[3];
	}
	else
	{
		obj[0] = 0;
		obj[1] = 0;
		obj[2] = 0;
		obj[3] = 0;
	}
}

bool write_rak15006(uint32_t memaddr, uint8_t *obj, uint16_t size)
{
	bool valid_addr = false;
	power_prefs.begin("Power", false);
	long_byte_s fram_val;
	fram_val.val_8[0] = obj[0];
	fram_val.val_8[1] = obj[1];
	fram_val.val_8[2] = obj[2];
	fram_val.val_8[3] = obj[3];
	if (memaddr == g_month_energy_addr)
	{
		power_prefs.putLong("m_e", fram_val.val_32);
		valid_addr = true;
	}
	if (memaddr == g_year_energy_addr)
	{
		power_prefs.putLong("y_e", fram_val.val_32);
		valid_addr = true;
	}
	power_prefs.end();
	return valid_addr;
}
