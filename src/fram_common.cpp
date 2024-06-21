/**
 * @file fram_common.cpp
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief FRAM functions, uses RAK15005 or RAk15006
 * @version 0.1
 * @date 2023-09-10
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include "main.h"

uint8_t fram_type = 0;

bool init_fram(void)
{
	if (init_rak15005())
	{
		fram_type = 1;
		myLog_d("Found FRAM RAK15005");
		return true;
	}
	else if (init_rak15006())
	{
		fram_type = 2;
		myLog_d("Found FRAM RAK15006");
		return true;
	}
	return false;
}

void read_fram(uint32_t memaddr, uint8_t *obj, uint16_t size)
{
	if (fram_type == 1)
	{
		read_rak15005(memaddr, obj, size);
	}
	if (fram_type == 2)
	{
		read_rak15006(memaddr, obj, size);
	}
}

bool write_fram(uint32_t memaddr, uint8_t *obj, uint16_t size)
{
	if (fram_type == 1)
	{
		return write_rak15005(memaddr, obj, size);
	}
	if (fram_type == 2)
	{
		return write_rak15006(memaddr, obj, size);
	}
	return false;
}
