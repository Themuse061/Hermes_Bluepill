#include <stdio.h>
#include <stdlib.h>
#include "usb_serial.h"
#include "USB_commands.h"

// Configuration
const char *COM_PORT = "COM14";
const int BAUD_RATE = 115200;

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
	USB_command_ping();

	// 3. Echo
	uint8_t echo_data[] = {0xDE, 0xAD, 0xBE, 0xEF};
	USB_command_echo(echo_data, sizeof(echo_data));

	// 4. I2C Write 1: 0x01 0x00 to 0x38
	uint8_t i2c_data1[] = {0x01, 0x00};
	USB_command_i2c_write(0x38, i2c_data1, sizeof(i2c_data1));

	// 5. Delay 1s
	printf("... Delaying 1s ...\n");
	// Using USB_wait_for_data with 1000ms timeout acts as a sleep if no data arrives
	USB_wait_for_data(1000);

	// 6. I2C Write 2: 0x01 0xFF to 0x38
	uint8_t i2c_data2[] = {0x01, 0xFF};
	USB_command_i2c_write(0x38, i2c_data2, sizeof(i2c_data2));

	//  I2C Write TO ch32 2: 0x01 0xFF to 0x38
	uint8_t i2c_data3[] = {0x00, 0xFF};

	//  I2C Write TO ch32 2: 0x01 0xFF to 0x38
	uint8_t i2c_data4[] = {0x00, 0x00};

	for (int i = 0; i < 10; i++)
	{
		printf("sending high\n");
		USB_command_i2c_write(0x09, i2c_data4, sizeof(i2c_data4));
		USB_wait_for_data(1000);

		printf("sending low\n");
		USB_command_i2c_write(0x09, i2c_data3, sizeof(i2c_data3));
		USB_wait_for_data(1000);
	}

	printf("--- Test Complete ---\n");

	// 7. Cleanup
	USB_deinit();

	return 0;
}