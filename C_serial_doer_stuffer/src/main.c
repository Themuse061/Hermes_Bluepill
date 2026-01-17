#include <stdio.h>
#include <stdlib.h>
#include <libserialport.h>

// Configuration
const char *COM_PORT = "COM14";
const int BAUD_RATE = 115200;

struct sp_port *port;

/**
 * Helper: Check libserialport return codes
 */
void check_sp(enum sp_return result, const char *msg)
{
	if (result < 0)
	{
		fprintf(stderr, "Error: %s\n", msg);
		exit(1);
	}
}

/**
 * Sends raw bytes to the MCU
 */
void USB_write(unsigned char *input_data, int length)
{
	printf("Sending: ");
	for (int i = 0; i < length; i++)
		printf("%02X ", input_data[i]);
	printf("\n");

	// Blocking write with 1000ms timeout
	int sent = sp_blocking_write(port, input_data, length, 1000);

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
	// sp_blocking_read returns the number of bytes read
	int bytes_read = sp_blocking_read(port, buffer, sizeof(buffer), 500);

	if (bytes_read > 0)
	{
		printf("MCU Response: ");
		for (int i = 0; i < bytes_read; i++)
		{
			printf("%02X ", buffer[i]);
		}
		printf("\n");
	}
	else if (bytes_read == 0)
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
	check_sp(sp_get_port_by_name(COM_PORT, &port), "Finding port");
	check_sp(sp_open(port, SP_MODE_READ_WRITE), "Opening port");

	// 2. Configure Port
	sp_set_baudrate(port, BAUD_RATE);
	sp_set_bits(port, 8);
	sp_set_parity(port, SP_PARITY_NONE);
	sp_set_stopbits(port, 1);

	// 3. Define the data to send: 0x04 0x04
	unsigned char cmd[] = {0x04, 0x04};

	// 4. Perform Transaction
	USB_write(cmd, sizeof(cmd));
	USB_read_and_display();

	// 5. Cleanup
	sp_close(port);
	sp_free_port(port);

	return 0;
}