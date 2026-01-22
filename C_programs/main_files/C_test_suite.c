#include <stdio.h>
#include <stdlib.h>
#include "usb_serial.h"
#include "USB_commands.h"
#include "USB_commands_high.h"
#include "Command_ID.h"
#include "helper.h"

// Configuration
const char *COM_PORT = "COM14";
const int BAUD_RATE = 115200;

#define TCA9554_is_connected 1
#define TCA9554_is_initialized 1
#define Hot_plate_is_connected 0
#define CH32V003_gpio_expander_is_conected 1

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
	USB_command_ping();

	// 3. Echo
	printf("Sending echo...\n");
	uint8_t echo_data[] = {0xDE, 0xAD, 0xBE, 0xEF};
	USB_command_echo(echo_data, sizeof(echo_data));

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
		}
		else
		{
			printf("omiting initializing");
		}
		delay_ms(1);
		TCA9554_write(0xFF);
		delay_ms(200);
		TCA9554_write(0x00);
		delay_ms(200);

		// 4.2 delay test
		uint8_t delay_packet[] = {
			// set to 0011
			5,
			USB_Device_Command_I2C_Write,
			TCA9554_ADDR,
			0x01,
			0b00000011,

			// Delay 200ms
			3,
			USB_Device_Command_Delay_Ms,
			200,

			// 1100
			5,
			USB_Device_Command_I2C_Write,
			TCA9554_ADDR,
			0x01,
			0b00001100,

			// Delay 200ms
			3,
			USB_Device_Command_Delay_Ms,
			200,

			// 0000
			5,
			USB_Device_Command_I2C_Write,
			TCA9554_ADDR,
			0x01,
			0x00,

		};
		USB_command_i2c_write(TCA9554_ADDR, delay_packet, sizeof(delay_packet));
		delay_ms(2000);
	}
	else
	{
		printf("TCA9554 Gpio expander isn't connected\n");
	}

	// 5. Hot plate EEPROM simulation test
	if (Hot_plate_is_connected)
	{
		printf("Testing Hot plate...\n");
		/*
		dummy1 ID - 0x03, size - 4
		dummy2 ID - 0x04, size - 8
		dummy3 ID - 0x05, size - 3
		*/

		uint8_t dummy_ID[] = {0x04};
		uint8_t test_data[] = {0xaa, 0xab, 0xc7, 0xc4};
		uint8_t temporary_data[9];
		temporary_data[0] = dummy_ID[0];
		for (int i = 1; i < sizeof(test_data) + 1; i++)
		{
			temporary_data[i] = test_data[i - 1];
		}

		USB_command_i2c_send_receive(Hot_plate_address, temporary_data, sizeof(temporary_data), 0, NULL); // write to register
		uint8_t data_back[9];
		USB_command_i2c_send_receive(Hot_plate_address, dummy_ID, 1, sizeof(test_data), data_back); // write to register

		// check if they are the same
		for (int i = 0; i < sizeof(test_data); i++)
		{
			if (test_data[i] = data_back[i])
			{
				printf("Good data\n");
			}
			else
			{
				printf("Bad data\n");
			}
		}
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

	return 0;
}