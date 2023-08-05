/**
 * @file RAK12002_rtc.cpp
 * @author Bernd Giesecke (bernd.giesecke@rakwireless.com)
 * @brief Initialization and usage of RAK12002 RTC module
 * @version 0.1
 * @date 2022-02-18
 *
 * @copyright Copyright (c) 2022
 *
 */
#include "main.h"
#include <Melopero_RV3028.h>

/** Instance of the RTC class */
Melopero_RV3028 rtc;

/** Date time structure */
date_time_s g_date_time;

/** Flag if RTC is set */
bool rtc_has_time = true;

uint8_t daily_production_addr = 0x00;
uint8_t monthly_production_addr = 0x10;
uint8_t test_addr = 0x20;

/**
 * @brief Initialize the RTC
 *
 * @return true if success
 * @return false if failed
 */
bool init_rak12002(void)
{
	Wire.begin();
	rtc.initI2C(Wire);

	rtc.writeToRegister(0x35, 0x00);
	rtc.writeToRegister(0x37, 0xB4); // Direct Switching Mode (DSM): when VDD < VBACKUP, switchover occurs from VDD to VBACKUP

	rtc.useEEPROM(false);

	rtc.set24HourMode(); // Set the device to use the 24hour format (default) instead of the 12 hour format

	g_date_time.year = rtc.getYear();
	g_date_time.month = rtc.getMonth();
	g_date_time.weekday = rtc.getWeekday();
	g_date_time.date = rtc.getDate();
	g_date_time.hour = rtc.getHour();
	g_date_time.minute = rtc.getMinute();
	g_date_time.second = rtc.getSecond();

	if ((g_date_time.year < 2022) || (g_date_time.year > 2100))
	{
		myLog_e("RTC not set or not available");
		rtc_has_time = false;
	}
	myLog_d("%d.%02d.%02d %d:%02d:%02d", g_date_time.year, g_date_time.month, g_date_time.date, g_date_time.hour, g_date_time.minute, g_date_time.second);

	// uint8_t test_data[4] = {0};

	// if (!write_rak12002_eeprom(daily_production_addr, test_data, 4))
	// {
	// 	myLog_e("Write to RTC EEPROM failed");
	// 	// return false;
	// }

	// read_rak12002_eeprom(daily_production_addr, test_data, 4);
	// for (uint8_t idx = 0; idx < 4; idx++)
	// {
	// 	myLog_e("EEPROM read from address %02X data %02X", idx, test_data[idx]);
	// }

	return true;
}

bool write_rak12002_eeprom(uint8_t address, uint8_t *data, uint8_t size)
{
	rtc.useEEPROM(true);

	time_t wait_start = millis();
	while (rtc.isEEPROMBusy())
	{
		delay(10);
		if ((millis() - wait_start) > 250)
		{
			rtc.useEEPROM(false);
			return false;
		}
	}

	for (uint8_t add_idx = 0; add_idx < size; add_idx++)
	{
		uint8_t eepr_addr = (uint8_t)(add_idx + address);
		rtc.writeEEPROMRegister(eepr_addr, data[add_idx]);
		myLog_e("EEPROM write to address %02X data %02X", eepr_addr, data[add_idx]);
		while (rtc.isEEPROMBusy())
		{
			delay(100);
		}
	}
	
	rtc.useEEPROM(false);
	delay(100);
	rtc.useEEPROM(true);

	uint8_t read_back[50];
	for (uint8_t add_idx = 0; add_idx < size; add_idx++)
	{
		uint8_t eepr_addr = (uint8_t)(add_idx + address);
		read_back[add_idx] = rtc.readEEPROMRegister(eepr_addr);
		myLog_e("EEPROM read from address %02X data %02X", eepr_addr, read_back[add_idx]);
		while (rtc.isEEPROMBusy())
		{
			delay(100);
		}
	}

	for (uint8_t idx = 0; idx < size; idx++)
	{
		if (read_back[idx] != data[idx])
		{
			myLog_e("EEPROM readback data failed at address %02X, wrote %02X read back %02X", address + idx, data[idx], read_back[idx]);
		}
	}
	rtc.useEEPROM(false);
	return true;
}

bool read_rak12002_eeprom(uint8_t address, uint8_t *data, uint8_t size)
{
	rtc.useEEPROM(true);

	time_t wait_start = millis();
	while (rtc.isEEPROMBusy())
	{
		delay(10);
		if ((millis() - wait_start) > 250)
		{
			rtc.useEEPROM(false);
			return false;
		}
	}

	for (uint8_t add_idx = 0; add_idx < size; add_idx++)
	{
		data[add_idx] = rtc.readEEPROMRegister(address + add_idx);
		delay(100);
	}

	rtc.useEEPROM(false);
	return true;
}

/**
 * @brief Set the RAK12002 date and time
 *
 * @param year in 4 digit format, e.g. 2020
 * @param month 1 to 12
 * @param date 1 to 31
 * @param hour 0 to 23
 * @param minute 0 to 59
 */
void set_rak12002(uint16_t year, uint8_t month, uint8_t date, uint8_t hour, uint8_t minute)
{
	uint8_t weekday = (date + (uint16_t)((2.6 * month) - 0.2) - (2 * (year / 100)) + year + (uint16_t)(year / 4) + (uint16_t)(year / 400)) % 7;
	myLog_d("RTC", "Calculated weekday is %d", weekday);
	rtc.setTime(year, month, weekday, date, hour, minute, 0);
}

/**
 * @brief Update g_data_time structure with current the date
 *        and time from the RTC
 *
 */
void read_rak12002(void)
{
	g_date_time.year = rtc.getYear();
	g_date_time.month = rtc.getMonth();
	g_date_time.weekday = rtc.getWeekday();
	g_date_time.date = rtc.getDate();
	g_date_time.hour = rtc.getHour();
	g_date_time.minute = rtc.getMinute();
	g_date_time.second = rtc.getSecond();
}