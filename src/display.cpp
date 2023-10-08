/**
 * @file display.cpp
 * @author Bernd Giesecke (bernd.giesecke@rakwireless.com)
 * @brief Display handling
 * @version 0.1
 * @date 2021-10-04
 *
 * @copyright Copyright (c) 2021
 *
 */
#include "main.h"
#include "my_font.h"

/** oled_disp instance **/
SSD1306Wire oled_disp(0x3c, SDA, SCL);

/** Buffer for line */
char disp_line1[32] = {0};
char disp_line2[32] = {0};
char disp_line3[32] = {0};
char disp_line4[32] = {0};

/** Alignment of text */
uint8_t set_alignment = TEXT_ALIGN_LEFT;

void write_header(void);
uint16_t find_greatest(uint16_t a, uint16_t b, uint16_t c, uint16_t d);

void init_display(void)
{
	oled_disp.init();
	// oled_disp.flipScreenVertically(); // clear the internal memory

	oled_disp.setFont(Monospaced_bold_10);
	snprintf(disp_line1, sizeof(disp_line1), "SMA Sunnyboy");
	oled_disp.drawString(0, 0, disp_line1);
	oled_disp.display(); // transfer internal memory to the display
}

void write_header(void)
{
	oled_disp.setFont(Monospaced_bold_10);
	char data[32] = {0};
	memset(data, 0, sizeof(data));
	snprintf(data, sizeof(data), "SMA Sunnyboy");
	oled_disp.drawString(0, 0, data);
}

void write_display(int power, int collected, int monthly)
{
	oled_disp.clear();
	uint8_t x_pos = 0;
	uint16_t str1_len = 0;
	uint16_t disp1_len = 0;
	uint16_t str2_len = 0;
	uint16_t disp2_len = 0;
	uint16_t str3_len = 0;
	uint16_t disp3_len = 0;
	uint16_t str4_len = 0;
	uint16_t disp4_len = 0;
	uint8_t longest_str = 0;
	uint16_t start_pos = 0;

	oled_disp.setTextAlignment(TEXT_ALIGN_LEFT);

	// update time
	read_rak12002();

	// Get longest string
	oled_disp.setFont(Monospaced_bold_10);
	str1_len = snprintf(disp_line1, sizeof(disp_line1), "SMA %s %d:%02d", isSuccess ? "O" : "X", g_date_time.hour, g_date_time.minute);
	disp1_len = oled_disp.getStringWidth(disp_line1, str1_len, true);

	oled_disp.setFont(Monospaced_bold_15);
	str2_len = snprintf(disp_line2, sizeof(disp_line2), "P: %dW", power);
	disp2_len = oled_disp.getStringWidth(disp_line2, str2_len, true);

	str3_len = snprintf(disp_line3, sizeof(disp_line3), "E: %dWh", collected);
	disp3_len = oled_disp.getStringWidth(disp_line3, str3_len, true);

	str4_len = snprintf(disp_line4, sizeof(disp_line4), "M: %.2fkWh", monthly / 1000.0);
	disp4_len = oled_disp.getStringWidth(disp_line4, str4_len, true);

	longest_str = find_greatest(disp1_len, disp2_len, disp3_len, disp4_len);

	switch (longest_str)
	{
	case 1:
		start_pos = disp1_len;
		break;
	case 2:
		start_pos = disp2_len;
		break;
	case 3:
		start_pos = disp3_len;
		break;
	case 4:
		start_pos = disp4_len;
		break;
	}

	// TEXT_ALIGN_LEFT, TEXT_ALIGN_CENTER, TEXT_ALIGN_RIGHT
	switch (set_alignment)
	{
	case 0:
		// oled_disp.setTextAlignment(TEXT_ALIGN_LEFT);
		x_pos = 0;
		set_alignment = 1;
		break;
	case 1:
		// oled_disp.setTextAlignment(TEXT_ALIGN_CENTER);
		x_pos = 64 - (start_pos / 2);
		set_alignment = 2;
		break;
	default:
		// oled_disp.setTextAlignment(TEXT_ALIGN_RIGHT);
		x_pos = 127 - start_pos;
		set_alignment = 0;
		break;
	}

	BLE_PRINTF("txt start %d", x_pos);

	oled_disp.setFont(Monospaced_bold_10);
	oled_disp.drawString(x_pos, 0, disp_line1);

	oled_disp.setFont(Monospaced_bold_15);
	oled_disp.drawString(x_pos, 12, disp_line2);

	oled_disp.drawString(x_pos, 29, disp_line3);

	oled_disp.drawString(x_pos, 46, disp_line4);

	oled_disp.display();
}

uint16_t find_greatest(uint16_t a, uint16_t b, uint16_t c, uint16_t d)
{
	if (a > b)
	{
		if (a > c)
		{
			if (a > d)
			{
				return 1;
			}
			else
			{
				return 4;
			}
		}
	}
	if (b > c)
	{
		if (b > d)
		{
			return 2;
		}
		else
		{
			return 4;
		}
	}
	if (c > d)
	{
		return 3;
	}
	return 4;
}
