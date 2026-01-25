#include <stdio.h>
#include <stdlib.h>
#include "Usb_driver.h"
#include "USB_commands.h"
#include "Command_ID.h"
#include "helper.h"
#include "hermes_packet_sending.h"
#include "C_test_suite_config.h"
#include <conio.h>

// uint8_t data
uint8_t ping_data[] = {0x09, 0x04, 0xFF, 0xaa, 0x00, 0x11, 0x00, 0xaa, 0xFF};
uint8_t echo_data[] = {0x17, 0x12, 0x10, 0xaa, 0xbb, 0xcc, 0xdd, 0x17, 0x02};
uint8_t TCA9554_init[] = {TCA9554_REG_CONFIG, 0x00};
uint8_t TCA9554_off[] = {TCA9554_REG_OUTPUT, 0x00};
uint8_t TCA9554_on[] = {TCA9554_REG_OUTPUT, 0b00001111};

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

	if (Test_ping_with_USB_writes)
	{

		printf("\n\n=========== Testing ping with USB writes ===========\n");
		uint8_t USB_raw_ping[] = {2, Command_ID_USB_Device_Ping};
		printf("sending: ");
		print_array_in_hex(USB_raw_ping, 2);

		USB_write(USB_raw_ping, 2);
		delay_ms(10);

		uint8_t USB_ping_read[9];
		USB_read(USB_ping_read, 9, 5000);

		printf("expe: ");
		print_array_in_hex(ping_data, sizeof(ping_data));
		printf("read: ");
		print_array_in_hex(USB_ping_read, 9);
	}

	if (Test_ping_with_Hermes_stack_and_manual_read)
	{

		printf("\n\n=========== Testing ping with Hermes stack and manual read ===========\n");

		printf("Current Stack Length %i\n", Hermes_Check_Stack_Length());
		printf("Current Stack Height %i\n", Hermes_Check_Stack_Height());

		printf("adding ping\n");
		Stack_add_ping();

		printf("Current Stack Length %i\n", Hermes_Check_Stack_Length());
		printf("Current Stack Height %i\n", Hermes_Check_Stack_Height());

		uint8_t ping_manual_read[sizeof(ping_data)];

		printf("Flushing Stack\n");
		Hermes_Flush_Stack();
		delay_ms(2000);
		printf("Current Stack Length %i\n", Hermes_Check_Stack_Length());
		printf("Current Stack Height %i\n", Hermes_Check_Stack_Height());
		Hermes_Read_Buffer_USB(ping_manual_read, sizeof(ping_manual_read));

		printf("expe: ");
		print_array_in_hex(ping_data, sizeof(ping_data));

		printf("read: ");
		print_array_in_hex(ping_manual_read, sizeof(ping_manual_read));
	}

	if (Test_ping_with_Hermes_flush_and_read)
	{

		printf("\n\n=========== Testing ping with Hermes flush and read ===========\n");

		printf("adding ping\n");
		Stack_add_ping();

		uint8_t ping_hermes_read[sizeof(ping_data)];
		Hermes_Flush_Stack_with_Read(ping_hermes_read, sizeof(ping_hermes_read));

		printf("expe: ");
		print_array_in_hex(ping_data, sizeof(ping_data));

		printf("read: ");
		print_array_in_hex(ping_hermes_read, sizeof(ping_hermes_read));
	}

	if (Test_echo)
	{

		printf("\n\n=========== Testing echo ===========\n");

		Stack_add_echo(echo_data, sizeof(echo_data));
		uint8_t echo_return_data[sizeof(echo_data)];

		Hermes_Flush_Stack_with_Read(echo_return_data, sizeof(echo_return_data) + 2);

		printf("sent: __ __ ");
		print_array_in_hex(echo_data, sizeof(echo_data));

		printf("read: ");
		print_array_in_hex(echo_return_data, sizeof(echo_return_data) + 2);
	}

	if (Test_TCA9554_I2C_write)
	{
		printf("\n\n=========== Testing I2C write with TCA9554 ===========\n");

		// init
		Stack_add_I2C_Write(TCA9554_ADDR, TCA9554_init, 2);

		// set to low
		Stack_add_I2C_Write(TCA9554_ADDR, TCA9554_off, 2);

		// send
		Hermes_Flush_Stack();
		delay_ms(2000);

		// Set high
		Stack_add_I2C_Write(TCA9554_ADDR, TCA9554_on, 2);
		Hermes_Flush_Stack();
		delay_ms(2000);

		// set low
		Stack_add_I2C_Write(TCA9554_ADDR, TCA9554_off, 2);
		Hermes_Flush_Stack();
		delay_ms(2000);

		// Set high
		Stack_add_I2C_Write(TCA9554_ADDR, TCA9554_on, 2);
		Hermes_Flush_Stack();
		delay_ms(2000);

		// set low
		Stack_add_I2C_Write(TCA9554_ADDR, TCA9554_off, 2);
		Hermes_Flush_Stack();
		delay_ms(2000);
	}

	if (Test_CH32V003_I2C_write_and_delay)
	{
		printf("\n\n=========== Testing I2C write with TCA9554 and delay ===========\n");
		Stack_add_I2C_Write(TCA9554_ADDR, TCA9554_init, 2);				   // init
		Stack_add_I2C_Write(TCA9554_ADDR, TCA9554_off, 2);				   // set low
		Stack_add_delay(200);											   // delay 200ms
		Stack_add_I2C_Write(TCA9554_ADDR, TCA9554_on, 2);				   // set high
		Stack_add_delay(200);											   // delay 200ms
		Stack_add_I2C_Write(TCA9554_ADDR, TCA9554_off, 2);				   // set low
		Stack_add_delay(200);											   // delay 200ms
		Stack_add_I2C_Write(TCA9554_ADDR, TCA9554_on, 2);				   // set high
		Stack_add_delay(200);											   // delay 200ms
		Stack_add_I2C_Write(TCA9554_ADDR, TCA9554_off, 2);				   // set low
		Stack_add_delay(200);											   // delay 200ms
		printf("Current stack height: %i\n", Hermes_Check_Stack_Height()); // print stack height
		Hermes_Flush_Stack();											   // flush stack
		delay_ms(2000);													   // wait for commands to execute
	}

	//  Cleanup
	printf("\n\n\n");
	USB_deinit();
	printf("\n--- Test Complete ---\n");

	// 8. wait for user input
	// printf("Press any key to exit...\n");
	// getch();

	return 0;
}
