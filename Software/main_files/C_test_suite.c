#include <stdio.h>
#include <stdlib.h>
#include "usb_serial.h"
#include "USB_commands.h"
#include "USB_commands_high.h"
#include "Command_ID.h"
#include "helper.h"
#include <conio.h>

#define TCA9554_is_connected 1
#define TCA9554_is_initialized 1
#define Hot_plate_is_connected 1
#define CH32V003_gpio_expander_is_conected 0

#define TCA9554_ADDR 0x38 // not used
#define Hot_plate_address 0x28
#define CH32V003_gpio_expander_address 0x09

int main()
{
	// 1. Setup Port
	if (USB_init(COM_PORT, BAUD_RATE) < 0)
	{
		fprintf(stderr, "Error: Failed to open port %s\n", COM_PORT);
		return 1;
	}

	printf("--- Starting USB Command Test ---\n");

	// 2. Ping
	printf("Sending Ping...\n");
	// USB_command_ping();

	// 3. Echo
	printf("Sending echo...\n");
	uint8_t echo_data[] = {0xDE, 0xAD, 0xBE, 0xEF};
	// USB_command_echo(echo_data, sizeof(echo_data));

	// 4. TCA test
	if (TCA9554_is_connected)
	{

		// 4.1 write test
		printf("Testing TCA9554...\n");
		printf("\n");
		if (!TCA9554_is_initialized)
		{
			printf("TCA9554 initializing\n");
			TCA9554_init();
			delay_ms(2000);
		}
		else
		{
			printf("omiting initializing\n");
		}

		printf("TCA 0x00");
		TCA9554_write(0x00);
		delay_ms(2000);

		printf("TCA 0xFF");
		TCA9554_write(0xFF);
		delay_ms(2000);

		printf("TCA 0x00");
		TCA9554_write(0x00);
		delay_ms(2000);

		// 4.2 delay test
		uint8_t delay_packet[] = {
			// set to 0011
			5,
			Command_ID_USB_Device_I2C_Write,
			TCA9554_ADDR,
			0x01,
			0b00000011,

			// Delay 200ms
			3,
			Command_ID_USB_Device_Delay_Ms,
			200,

			// 1100
			5,
			Command_ID_USB_Device_I2C_Write,
			TCA9554_ADDR,
			0x01,
			0b00001100,

			// Delay 200ms
			3,
			Command_ID_USB_Device_Delay_Ms,
			200,

			// 0000
			5,
			Command_ID_USB_Device_I2C_Write,
			TCA9554_ADDR,
			0x01,
			0x00,

		};
		USB_write(delay_packet, sizeof(delay_packet));
		delay_ms(5000);
	}
	else
	{
		printf("TCA9554 Gpio expander isn't connected\n");
	}

	//
	if (Hot_plate_is_connected)
	{
		printf("Testing Hot plate...\n");

		// Dummy 1: ID 0x03, Size 4
		uint8_t dummy1_id = 0x03;
		uint8_t dummy1_data[] = {0x11, 0x22, 0x33, 0x44};
		uint8_t dummy1_write_buf[5]; // ID + Data
		dummy1_write_buf[0] = dummy1_id;
		for (int i = 0; i < 4; i++)
			dummy1_write_buf[i + 1] = dummy1_data[i];

		// Dummy 2: ID 0x04, Size 8
		uint8_t dummy2_id = 0x04;
		uint8_t dummy2_data[] = {0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC};
		uint8_t dummy2_write_buf[9];
		dummy2_write_buf[0] = dummy2_id;
		for (int i = 0; i < 8; i++)
			dummy2_write_buf[i + 1] = dummy2_data[i];

		// Dummy 3: ID 0x05, Size 3
		uint8_t dummy3_id = 0x05;
		uint8_t dummy3_data[] = {0xDD, 0xEE, 0xFF};
		uint8_t dummy3_write_buf[4];
		dummy3_write_buf[0] = dummy3_id;
		for (int i = 0; i < 3; i++)
			dummy3_write_buf[i + 1] = dummy3_data[i];

		// Write all
		printf("Writing to Dummy 1...\n");
		USB_command_i2c_send_receive(Hot_plate_address, dummy1_write_buf, sizeof(dummy1_write_buf), 0, NULL);
		delay_ms(100);

		printf("Writing to Dummy 2...\n");
		USB_command_i2c_send_receive(Hot_plate_address, dummy2_write_buf, sizeof(dummy2_write_buf), 0, NULL);
		delay_ms(100);

		printf("Writing to Dummy 3...\n");
		USB_command_i2c_send_receive(Hot_plate_address, dummy3_write_buf, sizeof(dummy3_write_buf), 0, NULL);
		delay_ms(100);

		// Read and Check Dummy 1
		uint8_t read_buf1[4];
		USB_command_i2c_send_receive(Hot_plate_address, &dummy1_id, 1, sizeof(read_buf1), read_buf1);
		int d1_good = 1;
		for (int i = 0; i < sizeof(read_buf1); i++)
		{
			if (read_buf1[i] != dummy1_data[i])
				d1_good = 0;
		}
		if (d1_good)
			printf("dummy 1 good\n");
		else
			printf("dummy 1 bad\n");

		// Read and Check Dummy 2
		uint8_t read_buf2[8];
		USB_command_i2c_send_receive(Hot_plate_address, &dummy2_id, 1, sizeof(read_buf2), read_buf2);
		int d2_good = 1;
		for (int i = 0; i < sizeof(read_buf2); i++)
		{
			if (read_buf2[i] != dummy2_data[i])
				d2_good = 0;
		}
		if (d2_good)
			printf("dummy 2 good\n");
		else
			printf("dummy 2 bad\n");

		// Read and Check Dummy 3
		uint8_t read_buf3[3];
		USB_command_i2c_send_receive(Hot_plate_address, &dummy3_id, 1, sizeof(read_buf3), read_buf3);
		int d3_good = 1;
		for (int i = 0; i < sizeof(read_buf3); i++)
		{
			if (read_buf3[i] != dummy3_data[i])
				d3_good = 0;
		}
		if (d3_good)
			printf("dummy 3 good\n");
		else
			printf("dummy 3 bad\n");
	}
	else
	{
		printf("Hot plate isn't connected\n");
	}

	// 6. CH32V003 Gpio_expander test
	if (CH32V003_gpio_expander_is_conected)
	{
		printf("Testing CH32V003 Gpio...\n");

		for (int i = 0; i < 40; i++)
		{
			for (int j = 0; j < 8; j++)
			{
				ch32_small_expander_write(0x09, (uint8_t)j);
				USB_wait_for_data(40 - i);
			}
		}
	}
	else
	{
		printf("CH32V003 Gpio expander isn't connected\n");
	}

	printf("--- Test Complete ---\n");

	// 7. Cleanup
	USB_deinit();

	// 8. wait for user input
	getch();

	return 0;
}