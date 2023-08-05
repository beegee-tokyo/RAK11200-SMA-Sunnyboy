/**
 * @file rak15005_fram.cpp
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief Initialization, read and write functions for FRAM
 * @version 0.1
 * @date 2023-08-02
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "main.h"
#include <FRAM.h>

/** FRAM object */
FRAM32 fram;

bool init_rak15005(void)
{
	pinMode(WB_IO1, OUTPUT);
	digitalWrite(WB_IO1, LOW);

	Wire.begin();
	int rv = fram.begin(0x56);
	if (rv != 0)
	{
		myLog_e("INIT ERROR: %d", rv);
		return false;
	}

	myLog_d("ManufacturerID: %02X", fram.getManufacturerID());
	myLog_d("ProductID: %02X", fram.getProductID());
	myLog_d("memory size: %dkB", fram.getSize());

	// Erase FRAM area we use (only once!!!!!)
	// uint8_t eraser[16] = {0};
	// write_rak15005(0, eraser, 16);
	// fram.clear(0);
	// read_rak15005(0, eraser, 16);
	// for (int idx=0; idx < 16; idx++)
	// {
	// 	Serial.printf("%02X", eraser[idx]);
	// }
	// Serial.println("");

	return true;
}

void read_rak15005(uint32_t memaddr, uint8_t *obj, uint16_t size)
{
	fram.read(memaddr, obj, size);
}

bool write_rak15005(uint32_t memaddr, uint8_t *obj, uint16_t size)
{
	uint8_t comp_buff[size];
	fram.write(memaddr, obj, size);

	fram.read(memaddr, comp_buff, size);

	for (int idx = 0; idx < size; idx++)
	{
		if (obj[idx] != comp_buff[idx])
		{
			myLog_e("Error checking written data. Found %02X expected %02X", comp_buff[idx], obj[idx]);
			return false;
		}
	}
	return true;
}
