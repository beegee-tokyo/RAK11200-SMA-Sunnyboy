/**
 * Example: SMAReader_Demo.ino
 *
 */
#include "main.h"

/** Address of SMA Sunnyboy Inverter */
IPAddress inverterIP(192, 168, 1, 185);

/** SMA value reader */
SMAReader smaReader(inverterIP, SMAREADER_USER, INVERTERPWD, 5);

/** SW version of the gateway */
char g_sw_version[10];

/** Flag if data from SMA could be read */
bool isSuccess = false;

/** LoRaWAN packet */
CayenneLPP g_solution_data(255);

bool daily_added = false;
uint32_t g_month_energy = 0;
uint32_t g_year_energy = 0;
uint32_t g_month_energy_addr = 0x00;
uint32_t g_year_energy_addr = 0x04;

long_byte_s fram_val;

/**
 * @brief Arduino setup
 *
 */
void setup()
{
	// LoRa module & FRAM uses 3V3_S, need to enable power supply over WB_IO2
	pinMode(WB_IO2, OUTPUT);
	digitalWrite(WB_IO2, HIGH);

	Serial.begin(115200);
	pinMode(LED_BLUE, OUTPUT);
	pinMode(LED_GREEN, OUTPUT);
	digitalWrite(LED_GREEN, HIGH);
	digitalWrite(LED_BLUE, HIGH);

	// Scan the I2C interfaces for devices
	byte error;
	Wire.begin();
	for (byte address = 1; address < 127; address++)
	{
		Wire.beginTransmission(address);
		error = Wire.endTransmission();
		if (error == 0)
		{
			Serial.printf("Found sensor at I2C1 0x%02X\n", address);
		}
	}

	start_check_serial();

	init_rak12002();

	init_fram();

	fram_val.val_32 = 0;
	read_fram(g_month_energy_addr, fram_val.val_8, 4);
	g_month_energy = fram_val.val_32;
	myLog_d("Got month energy %ld", g_month_energy);
	if (g_month_energy == 11751)
	{
		g_month_energy = 28916;
		fram_val.val_32 = g_month_energy;
		write_fram(g_month_energy_addr, fram_val.val_8, 4);
	}

	fram_val.val_32 = 0;
	read_fram(g_year_energy_addr, fram_val.val_8, 4);
	g_year_energy = fram_val.val_32;
	myLog_d("Got year energy %ld", g_year_energy);
	if (g_year_energy == 0)
	{
		g_year_energy = 77504;
		fram_val.val_32 = g_year_energy;
		write_fram(g_year_energy_addr, fram_val.val_8, 4);
	}

	read_rak12002();

	init_display();

	init_batt();

	init_wifi();
	if (g_has_credentials)
	{
		initOTA();
	}

	// Initialize RAK13300 module
	if (init_lorawan() != 0)
	{
		myLog_e("Failed to initialize RAK13300");
	}

	// Initialize BLE interface
	init_ble();
}

/**
 * @brief Arduino loop
 *
 */
void loop()
{
	// Handle OTA updates
	ArduinoOTA.handle();

	if (g_ota_running)
	{
		return;
	}

	if (!WiFi.isConnected())
	{
		wifi_multi.run();
	}

	// wait for WiFi connection
	if (WiFi.isConnected() && g_lpwan_has_joined)
	{
		digitalWrite(LED_GREEN, HIGH);
		// Get Power and Today's Energy values from Sunnyboy

		String keys[2] = {KEY_POWER, KEY_ENERGY_TODAY};
		int values[2] = {0, 0};
		isSuccess = false;
		int retry_count = 0;

		read_rak12002();
		myLog_d("%d.%02d.%02d %d:%02d:%02d", g_date_time.year, g_date_time.month, g_date_time.date, g_date_time.hour, g_date_time.minute, g_date_time.second);

		while (!isSuccess)
		{
			myLog_d("Heap: %ld", ESP.getFreeHeap());
			myLog_d("Stack: %ld", uxTaskGetStackHighWaterMark(NULL));

			delay(500);
			isSuccess = smaReader.getValues(2, keys, values);
			myLog_d("Getting values: %s", isSuccess ? "success" : "fail");
			BLE_PRINTF("Getting values: %s", isSuccess ? "success" : "fail");

			if (isSuccess && (values[0] < 3000))
			{
				// at night the current value turns to -1
				if (values[0] == -1)
				{
					values[0] = 0;
				}

				write_display(values[0], values[1]);

				myLog_d("Current power %d W - Collected today %d Wh", values[0], values[1]);
				BLE_PRINTF("Current power %d W - Collected today %d Wh", values[0], values[1]);

				// Reset the packet
				g_solution_data.reset();

				g_solution_data.addPower(0x01, (uint32_t)values[0]);
				g_solution_data.addEnergy(0x01, (float)values[1] / 1000.0);

				// If it is 23:58, add the daily collection to the monthly collection
				if ((g_date_time.hour == 23) && (g_date_time.minute >= 50) && !daily_added)
				{
					myLog_d("Add daily production to month");
					BLE_PRINTF("Add daily production to month");
					// Send today's production
					g_solution_data.addEnergy(0x02, (float)values[1] / 1000.0);

					// Update monthly production
					BLE_PRINTF("Old month %.3f Wh", (float)g_month_energy / 1000.0);
					g_month_energy = g_month_energy + values[1];
					BLE_PRINTF("New month %.3f Wh", (float)g_month_energy / 1000.0);
					// Save last value in FRAM
					fram_val.val_32 = g_month_energy;
					write_fram(g_month_energy_addr, fram_val.val_8, 4);
					fram_val.val_32 = g_year_energy;
					write_fram(g_year_energy_addr, fram_val.val_8, 4);
					daily_added = true;

					// Check if it is the last day of the month
					bool save_send_month = false;
					switch (g_date_time.month)
					{
					case 1:
					case 3:
					case 5:
					case 7:
					case 8:
					case 10:
					case 12:
						if (g_date_time.date == 31)
						{
							save_send_month = true;
						}
						break;
					case 4:
					case 6:
					case 9:
					case 11:
						if (g_date_time.date == 30)
						{
							save_send_month = true;
						}
						break;
					case 2:
						uint16_t check_leap_year = g_date_time.year % 4;
						if (check_leap_year != 0)
						{
							if (g_date_time.date == 28)
							{
								save_send_month = true;
							}
						}
						else
						{
							if (g_date_time.date == 29)
							{
								save_send_month = true;
							}
						}
						break;
					}
					if (save_send_month)
					{
						myLog_d("New month is starting");
						BLE_PRINTF("New month is starting");
						// Add monthly production to yearly production
						BLE_PRINTF("Old year %.3f Wh", (float)g_year_energy / 1000.0);
						g_year_energy = g_year_energy + g_month_energy;
						BLE_PRINTF("New year %.3f Wh", (float)g_year_energy / 1000.0);

						// Save last value in FRAM
						fram_val.val_32 = g_year_energy;
						write_fram(g_year_energy_addr, fram_val.val_8, 4);

						// Send monthly production
						g_solution_data.addEnergy(0x03, (float)g_month_energy / 1000.0);
						// Clear monthly production
						g_month_energy = 0;

						// Save last value in FRAM
						fram_val.val_32 = g_month_energy;
						write_fram(g_month_energy_addr, fram_val.val_8, 4);
					}
					if ((g_date_time.month == 12) && (g_date_time.date == 31))
					{
						myLog_d("New year is starting");
						BLE_PRINTF("New year is starting");
						// Send yearly production
						g_solution_data.addEnergy(0x04, (float)g_year_energy / 1000.0);
						// Clear yearly production
						g_year_energy = 0;

						// Save last value in FRAM
						fram_val.val_32 = g_year_energy;
						write_fram(g_year_energy_addr, fram_val.val_8, 4);
					}
				}
				// If it is 00:00, clear the daily_added flag
				if ((g_date_time.hour == 0) && daily_added)
				{
					myLog_d("Reset daily_added");
					daily_added = false;
				}

				lmh_error_status result = send_lora_packet(g_solution_data.getBuffer(), g_solution_data.getSize());
				switch (result)
				{
				case LMH_SUCCESS:
					myLog_d("Packet enqueued");
					BLE_PRINTF("Packet enqueued");
					break;
				case LMH_BUSY:
					myLog_e("LoRa transceiver is busy");
					BLE_PRINTF("LoRa transceiver is busy");
					break;
				case LMH_ERROR:
					myLog_e("Packet error, too big to send with current DR");
					BLE_PRINTF("Packet error, too big to send with current DR");
					break;
				}
			}
			else if (values[0] >= 3000)
			{
				myLog_e("Values not valid");
				BLE_PRINTF("Values not valid");
			}
			else
			{
				retry_count++;
				myLog_e("Failed to read data from SMA inverter");
				BLE_PRINTF("Failed to read data from SMA inverter");
				if (retry_count == 5)
				{
					myLog_e("Failed 5 times to read data from SMA inverter, giving up");

					// Reset the packet
					g_solution_data.reset();

					bool send_anyway = false;

					/// Check for month/year ending and save data even if reading from SMA inverter failed
					// If it is 23:58, add the daily collection to the monthly collection
					if ((g_date_time.hour == 23) && (g_date_time.minute >= 50) && !daily_added)
					{
						myLog_d("Add daily production to month");
						BLE_PRINTF("Add daily production to month");

						send_anyway = true;

						// Update monthly production
						BLE_PRINTF("Old month %.3f Wh", (float)g_month_energy / 1000.0);
						g_month_energy = g_month_energy + values[1];
						BLE_PRINTF("New month %.3f Wh", (float)g_month_energy / 1000.0);
						// Save last value in FRAM
						fram_val.val_32 = g_month_energy;
						write_fram(g_month_energy_addr, fram_val.val_8, 4);
						fram_val.val_32 = g_year_energy;
						write_fram(g_year_energy_addr, fram_val.val_8, 4);
						daily_added = true;

						// Check if it is the last day of the month
						bool save_send_month = false;
						switch (g_date_time.month)
						{
						case 1:
						case 3:
						case 5:
						case 7:
						case 8:
						case 10:
						case 12:
							if (g_date_time.date == 31)
							{
								save_send_month = true;
							}
							break;
						case 4:
						case 6:
						case 9:
						case 11:
							if (g_date_time.date == 30)
							{
								save_send_month = true;
							}
							break;
						case 2:
							uint16_t check_leap_year = g_date_time.year % 4;
							if (check_leap_year != 0)
							{
								if (g_date_time.date == 28)
								{
									save_send_month = true;
								}
							}
							else
							{
								if (g_date_time.date == 29)
								{
									save_send_month = true;
								}
							}
							break;
						}
						if (save_send_month)
						{
							myLog_d("New month is starting");
							BLE_PRINTF("New month is starting");
							// Add monthly production to yearly production
							BLE_PRINTF("Old year %.3f Wh", (float)g_year_energy / 1000.0);
							g_year_energy = g_year_energy + g_month_energy;
							BLE_PRINTF("New year %.3f Wh", (float)g_year_energy / 1000.0);

							// Save last value in FRAM
							fram_val.val_32 = g_year_energy;
							write_fram(g_year_energy_addr, fram_val.val_8, 4);

							// Send monthly production
							g_solution_data.addEnergy(0x03, (float)g_month_energy / 1000.0);

							// Clear monthly production
							g_month_energy = 0;

							// Save last value in FRAM
							fram_val.val_32 = g_month_energy;
							write_fram(g_month_energy_addr, fram_val.val_8, 4);
						}
						if ((g_date_time.month == 12) && (g_date_time.date == 31))
						{
							send_anyway = true;

							myLog_d("New year is starting");
							BLE_PRINTF("New year is starting");
							// Send yearly production
							g_solution_data.addEnergy(0x04, (float)g_year_energy / 1000.0);
							// Clear yearly production
							g_year_energy = 0;

							// Save last value in FRAM
							fram_val.val_32 = g_year_energy;
							write_fram(g_year_energy_addr, fram_val.val_8, 4);
						}
						if (send_anyway)
						{
							lmh_error_status result = send_lora_packet(g_solution_data.getBuffer(), g_solution_data.getSize());
							switch (result)
							{
							case LMH_SUCCESS:
								myLog_d("Packet enqueued");
								BLE_PRINTF("Packet enqueued");
								break;
							case LMH_BUSY:
								myLog_e("LoRa transceiver is busy");
								BLE_PRINTF("LoRa transceiver is busy");
								break;
							case LMH_ERROR:
								myLog_e("Packet error, too big to send with current DR");
								BLE_PRINTF("Packet error, too big to send with current DR");
								break;
							}
						}
					}

					break;
				}
				// Retry in 15 seconds
				time_t start_delay = millis();
				while ((millis() - start_delay) < 15000)
				{
					// Handle OTA updates
					ArduinoOTA.handle();
					// if (g_ota_running)
					// {
					// 	return;
					// }
					delay(100);
				}
			}
			// Handle OTA updates
			ArduinoOTA.handle();
			if (g_ota_running)
			{
				return;
			}
		}

		if (!isSuccess)
		{
			write_display(values[0], values[1]);
		}

		digitalWrite(LED_GREEN, LOW);

		time_t start_delay = millis();
		while ((millis() - start_delay) < g_lorawan_settings.send_repeat_time)
		{
			// Handle OTA updates
			ArduinoOTA.handle();
			if (g_ota_running)
			{
				return;
			}
			delay(500);
		}
	}
	else
	{
		if (!WiFi.isConnected())
		{
			myLog_d("WiFi not connected");
			BLE_PRINTF("WiFi not connected");
		}
		delay(5000);
	}
}
