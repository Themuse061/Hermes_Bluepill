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

uint8_t dummy1_ID_in_uint8[] = {dummy1_ID};
uint8_t dummy1_data_read1[dummy1_len + 4];
uint8_t dummy1_data_read2[dummy1_len + 4];
uint8_t dummy1_data_write_1[] = {dummy1_ID, 0x01, 0x02, 0x03, 0x04};
uint8_t dummy1_data_write_2[] = {dummy1_ID, 0x11, 0x12, 0x13, 0x14};

uint8_t dummy2_ID_in_uint8[] = {dummy2_ID};
uint8_t dummy2_data_read1[dummy2_len + 4];
uint8_t dummy2_data_read2[dummy2_len + 4];
uint8_t dummy2_data_write_1[] = {dummy2_ID, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
uint8_t dummy2_data_write_2[] = {dummy2_ID, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18};

uint8_t dummy3_ID_in_uint8[] = {dummy3_ID};
uint8_t dummy3_data_read1[dummy3_len + 4];
uint8_t dummy3_data_read2[dummy3_len + 4];
uint8_t dummy3_data_write_1[] = {dummy3_ID, 0x01, 0x02, 0x03};
uint8_t dummy3_data_write_2[] = {dummy3_ID, 0x11, 0x12, 0x13};

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

	if (Test_hot_plate_send_recieve)
	{

		printf("\n\n=========== Testing I2C send recieve with hotplate ===========\n");

		// Write first set of data
		Stack_add_I2C_Write(Hot_plate_address, dummy1_data_write_1, sizeof(dummy1_data_write_1));
		Stack_add_delay(50);
		Stack_add_I2C_Write(Hot_plate_address, dummy2_data_write_1, sizeof(dummy2_data_write_1));
		Stack_add_delay(50);
		Stack_add_I2C_Write(Hot_plate_address, dummy3_data_write_1, sizeof(dummy3_data_write_1));
		Stack_add_delay(50);
		Hermes_Flush_Stack();
		delay_ms(2000);

		// Read first set of data
		Stack_add_I2C_Send_recieve(Hot_plate_address, 1, dummy1_len, dummy1_ID_in_uint8);
		Hermes_Flush_Stack_with_Read(dummy1_data_read1, sizeof(dummy1_data_read1));

		Stack_add_I2C_Send_recieve(Hot_plate_address, 1, dummy2_len, dummy2_ID_in_uint8);
		Hermes_Flush_Stack_with_Read(dummy2_data_read1, sizeof(dummy2_data_read1));

		Stack_add_I2C_Send_recieve(Hot_plate_address, 1, dummy3_len, dummy3_ID_in_uint8);
		Hermes_Flush_Stack_with_Read(dummy3_data_read1, sizeof(dummy3_data_read1));

		// Write second set of data
		Stack_add_I2C_Write(Hot_plate_address, dummy1_data_write_2, sizeof(dummy1_data_write_2));
		Stack_add_delay(50);
		Stack_add_I2C_Write(Hot_plate_address, dummy2_data_write_2, sizeof(dummy2_data_write_2));
		Stack_add_delay(50);
		Stack_add_I2C_Write(Hot_plate_address, dummy3_data_write_2, sizeof(dummy3_data_write_2));
		Stack_add_delay(50);
		Hermes_Flush_Stack();
		delay_ms(2000);

		// Read second set of data
		Stack_add_I2C_Send_recieve(Hot_plate_address, 1, dummy1_len, dummy1_ID_in_uint8);
		Hermes_Flush_Stack_with_Read(dummy1_data_read2, sizeof(dummy1_data_read2));

		Stack_add_I2C_Send_recieve(Hot_plate_address, 1, dummy2_len, dummy2_ID_in_uint8);
		Hermes_Flush_Stack_with_Read(dummy2_data_read2, sizeof(dummy2_data_read2));

		Stack_add_I2C_Send_recieve(Hot_plate_address, 1, dummy3_len, dummy3_ID_in_uint8);
		Hermes_Flush_Stack_with_Read(dummy3_data_read2, sizeof(dummy3_data_read2));

		// Display results
		printf("Dummy 1:\n");
		printf("	write 1: __ __ __ ");
		print_array_in_hex(dummy1_data_write_1, sizeof(dummy1_data_write_1));
		printf("	read_ 1: ");
		print_array_in_hex(dummy1_data_read1, sizeof(dummy1_data_read1));
		printf("	write 2: __ __ __ ");
		print_array_in_hex(dummy1_data_write_2, sizeof(dummy1_data_write_2));
		printf("	read_ 2: ");
		print_array_in_hex(dummy1_data_read2, sizeof(dummy1_data_read2));
		printf("Dummy 2:\n");
		printf("	write 1: __ __ __ ");
		print_array_in_hex(dummy2_data_write_1, sizeof(dummy2_data_write_1));
		printf("	read_ 1: ");
		print_array_in_hex(dummy2_data_read1, sizeof(dummy2_data_read1));
		printf("	write 2: __ __ __ ");
		print_array_in_hex(dummy2_data_write_2, sizeof(dummy2_data_write_2));
		printf("	read_ 2: ");
		print_array_in_hex(dummy2_data_read2, sizeof(dummy2_data_read2));
		printf("Dummy 3:\n");
		printf("	write 1: __ __ __ ");
		print_array_in_hex(dummy3_data_write_1, sizeof(dummy3_data_write_1));
		printf("	read_ 1: ");
		print_array_in_hex(dummy3_data_read1, sizeof(dummy3_data_read1));
		printf("	write 2: __ __ __ ");
		print_array_in_hex(dummy3_data_write_2, sizeof(dummy3_data_write_2));
		printf("	read_ 2: ");
		print_array_in_hex(dummy3_data_read2, sizeof(dummy3_data_read2));
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
