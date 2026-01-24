#include <stdio.h>
#include <stdlib.h>
#include "Usb_driver.h"
#include "USB_commands.h"
#include "Command_ID.h"
#include "helper.h"
#include "hermes_packet_sending.h"
#include <conio.h>

#define TCA9554_REG_OUTPUT 0x01
#define TCA9554_REG_CONFIG 0x03

#define TCA9554_is_connected 1
#define TCA9554_is_initialized 1
#define Hot_plate_is_connected 1
#define CH32V003_gpio_expander_is_conected 0

#define TCA9554_ADDR 0x38 // not used
#define Hot_plate_address 0x28
#define CH32V003_gpio_expander_address 0x09

// Ideally, pass these as arguments to USB_init
const char *COM_PORT = "COM14";
const int BAUD_RATE = 115200;

void print_array_in_hex(uint8_t *array, int len)
{
	for (int i = 0; i < len; i++)
	{
		printf("%02X ", array[i]);
	}
	printf("\n");
}

int main()
{
	// 1. Setup Port
	// Note: USB_init logic remains same (helper/driver level)
	if (USB_init(COM_PORT, BAUD_RATE) < 0)
	{
		fprintf(stderr, "Error: Failed to open USB port");
		return 1;
	}
	printf("\n\n");
	printf("=========== Testing ping with USB writes ===========\n");
	uint8_t USB_raw_ping[] = {2, Command_ID_USB_Device_Ping};
	printf("sending: ");
	print_array_in_hex(USB_raw_ping, 2);

	USB_write(USB_raw_ping, 2);
	delay_ms(10);

	uint8_t USB_ping_read[10];
	USB_read(USB_ping_read, 10, 5000);
	printf("reading: ");
	print_array_in_hex(USB_ping_read, 10);

	printf("\n\n");
	printf("=========== Testing ping with Hermes stack ===========\n");

	printf("Current Stack Height %i\n", Hermes_Check_Stack_Length());
	printf("Current Stack Height %i\n", Hermes_Check_Stack_Height());

	printf("adding ping\n");
	Stack_add_ping();

	printf("Current Stack Height %i\n", Hermes_Check_Stack_Length());
	printf("Current Stack Height %i\n", Hermes_Check_Stack_Height());

	uint8_t ping_data[] = {0x09, 0x04, 0xFF, 0xaa, 0x00, 0x11, 0x00, 0xaa, 0xFF};
	uint8_t ping_read[sizeof(ping_data)];

	printf("Flushing Stack\n");
	Hermes_Flush_Stack();
	delay_ms(2000);
	printf("Current Stack Height %i\n", Hermes_Check_Stack_Length());
	printf("Current Stack Height %i\n", Hermes_Check_Stack_Height());
	Hermes_Read_Buffer_USB(ping_read, sizeof(ping_read));

	printf("read: ");
	print_array_in_hex(ping_read, sizeof(ping_read));

	printf("--- Test Complete ---\n");

	// 7. Cleanup
	USB_deinit();

	// 8. wait for user input
	// printf("Press any key to exit...\n");
	// getch();

	return 0;
}
