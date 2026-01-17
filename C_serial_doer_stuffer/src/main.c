#include <stdio.h>
#include <stdlib.h>
#include "usb_serial.h"

// Configuration
const char *COM_PORT = "COM14";
const int BAUD_RATE = 115200;

/**
 * Sends raw bytes to the MCU with logging
 */
void app_log_and_send(unsigned char *input_data, int length)
{
	printf("Sending: ");
	for (int i = 0; i < length; i++)
		printf("%02X ", input_data[i]);
	printf("\n");

	// Use new abstraction
	int sent = USB_write(input_data, length);

	if (sent != length)
	{
		printf("Error: Sent %d of %d bytes.\n", sent, length);
	}
}

/**
 * Waits for and prints the MCU response
 */
void USB_read_and_display()
{
	unsigned char buffer[512];

	// Wait for up to 500ms for a response
	int bytes_read = USB_read(buffer, sizeof(buffer), 500);

	if (bytes_read > 0)
	{
		printf("MCU Response: ");
		for (int i = 0; i < bytes_read; i++)
		{
			printf("%02X ", buffer[i]);
		}
		printf("\n");
	}
	else if (bytes_read == 0) // Note: USB_read (sp_blocking_read) might return 0 on timeout depending on lib version, or error
	{
		printf("MCU Response: (Timeout/No data)\n");
	}
	else
	{
		printf("MCU Response: (Error reading port)\n");
	}
}

int main()
{
	// 1. Setup Port
	if (USB_init(COM_PORT, BAUD_RATE) < 0) {
		fprintf(stderr, "Error: Failed to open port %s\n", COM_PORT);
		return 1;
	}

	// 2. Define the data to send: 0x04 0x04
	unsigned char cmd[] = {0x04, 0x04};

	// 3. Perform Transaction
	app_log_and_send(cmd, sizeof(cmd));
	USB_read_and_display();

	// 4. Cleanup
	USB_deinit();

	return 0;
}