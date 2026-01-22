#include <stdio.h>
#include <stdlib.h>
#include "usb_serial.h"
#include "USB_commands.h"
#include "USB_commands_high.h"

// Configuration
const char *COM_PORT = "COM14";
const int BAUD_RATE = 115200;

#define TCA9554_ADDR 0x38

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

	// 4. TCA test
	TCA9554_write(0x00);

	// 5. Delay 1s
	printf("... Delaying 1s ...\n");
	// Using USB_wait_for_data with 1000ms timeout acts as a sleep if no data arrives
	USB_wait_for_data(2000);

	// 6. TCA Write 2:
	TCA9554_write(0xFF);

	//  I2C Write TO ch32

	for (int i = 0; i < 40; i++)
	{
		for (int j = 0; j < 8; j++)
		{
			// i2c_data3[1] = j;
			printf("sending new value: %d\n", j);
			// USB_command_i2c_write(0x09, i2c_data3, sizeof(i2c_data2));
			ch32_small_expander_write(0x09, (uint8_t)j);
			USB_wait_for_data(40 - i);
		}
	}

	printf("--- Test Complete ---\n");

	// 7. Cleanup
	USB_deinit();

	return 0;
}