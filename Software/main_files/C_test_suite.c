#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "helper.h"				 // for delay and timer
#include "hermes_header.h"		 // for entire hermes
#include "Command_ID.h"			 // global include with command ID's
#include "C_test_suite_config.h" // Config for this source

// memcpy data
uint8_t *destination;
uint8_t *source;
int length;

// uint8_t data
uint8_t ping_data[] = {0x09, 0x04, 0xFF, 0xaa, 0x00, 0x11, 0x00, 0xaa, 0xFF};

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

// Ideally, pass these as arguments to hermes_USB_init
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
	// Note: hermes_USB_init logic remains same (helper/driver level)
	if (hermes_USB_init(COM_PORT, BAUD_RATE) < 0)
	{
		fprintf(stderr, "Error: Failed to open USB port");
		return 1;
	}
	helper_start_timer(1);

	if (Test_ping_with_hermes_USB_sends)
	{

		printf("\n\n=========== Testing ping with USB writes ===========\n");
		uint8_t USB_raw_ping[] = {2, Command_ID_USB_Device_Ping};
		printf("sending: ");
		print_array_in_hex(USB_raw_ping, 2);

		hermes_USB_send(USB_raw_ping, 2);
		delay_ms(10);
		printf("bytes in USB buffer: %i", hermes_USB_check_recieve_buffer());

		uint8_t USB_ping_read[9];
		hermes_USB_recieve(USB_ping_read, 9);

		printf("expe: ");
		print_array_in_hex(ping_data, sizeof(ping_data));
		printf("read: ");
		print_array_in_hex(USB_ping_read, 9);
		printf("bytes in USB buffer: %i", hermes_USB_check_recieve_buffer());
	}

	if (Test_ping_with_Hermes_stack_and_manual_read)
	{

		printf("\n\n=========== Testing ping with Hermes stack and manual read ===========\n");

		printf("Current Stack Height %i\n", hermes_packet_check_heigh_taken());

		printf("adding ping\n");
		hermes_add_ping();

		printf("Current Stack Height %i\n", hermes_packet_check_heigh_taken());

		uint8_t ping_manual_read[sizeof(ping_data)];

		printf("Flushing Stack\n");
		hermes_packet_flush_blind();
		printf("Current Stack Height %i\n", hermes_packet_check_heigh_taken());
		delay_ms(2000);
		hermes_USB_recieve(ping_manual_read, sizeof(ping_manual_read));

		printf("expe: ");
		print_array_in_hex(ping_data, sizeof(ping_data));

		printf("read: ");
		print_array_in_hex(ping_manual_read, sizeof(ping_manual_read));
	}

	if (Test_ping_with_Hermes_parse_USB)
	{

		printf("\n\n=========== Test_ping_with_Hermes_parse_USB ===========\n");

		printf("adding ping\n");
		hermes_add_ping();

		hermes_packet_flush_blind();
		delay_ms(2000);
		hermes_packet_parse_USB();

		printf("expe: ");
		print_array_in_hex(ping_data, sizeof(ping_data));

		printf("read: ");
		print_array_in_hex(hermes_recieve_buffer[0], hermes_recieve_buffer[0][0]);
	}

	if (Test_ping_with_Hermes_parse_USB_multiple_pings)
	{
		printf("\n\n=========== Test_ping_with_Hermes_parse_USB_multiple_pings ===========\n");

		printf("adding 5 pings\n");
		hermes_add_ping();
		hermes_add_ping();
		hermes_add_ping();
		hermes_add_ping();
		hermes_add_ping();

		printf("flushing blindly\n");
		hermes_packet_flush_blind();

		printf("parses data");
		delay_ms(5000);
		hermes_packet_parse_USB();

		printf("we've recieved\n");
		print_array_in_hex(hermes_recieve_buffer[0], 16);
		print_array_in_hex(hermes_recieve_buffer[1], 16);
		print_array_in_hex(hermes_recieve_buffer[2], 16);
		print_array_in_hex(hermes_recieve_buffer[3], 16);
		print_array_in_hex(hermes_recieve_buffer[4], 16);
	}

	if (Test_echo_add)
	{

		printf("\n\n=========== Test_echo_add ===========\n");

		uint8_t echo_data[] = {0x17, 0x12, 0x10, 0xaa, 0xbb, 0xcc, 0xdd, 0x17, 0x02};

		hermes_add_echo(echo_data, sizeof(echo_data));

		hermes_packet_flush_blind();
		delay_ms(2000);
		hermes_packet_parse_USB();

		printf("sent: __ __ ");
		print_array_in_hex(echo_data, sizeof(echo_data));

		printf("read: ");
		print_array_in_hex(hermes_recieve_buffer[0], sizeof(echo_data) + 2);
	}

	if (Test_hermes_flush)
	{
		printf("\n\n=========== Test_hermes_flush ===========\n");

		for (int i = 0; i < 5; i++)
		{
			hermes_add_ping();
		}

		printf("starting timer\n");
		helper_start_timer(2);
		printf("flushing buffer\n");
		hermes_packet_flush();
		printf("buffer flushed\n");
		helper_end_timer(2);

		printf("read data:\n");
		for (int i = 0; i < HERMES_BUFFER_RECIEVE_STACK_MAX_HEIGHT; i++)
		{
			printf("%02d: ", i);
			print_array_in_hex(hermes_recieve_buffer[i], 16);
		}
	}

	if (Test_hermes_flush)
	{
		printf("\n\n=========== Test_hermes_flush_long ===========\n");

		for (int i = 0; i < 15; i++)
		{
			hermes_add_ping();
		}

		printf("starting timer\n");
		helper_start_timer(2);
		printf("flushing buffer\n");
		hermes_packet_flush();
		printf("buffer flushed\n");
		helper_end_timer(2);

		printf("read data:\n");
		for (int i = 0; i < HERMES_BUFFER_RECIEVE_STACK_MAX_HEIGHT; i++)
		{
			printf("%02d: ", i);
			print_array_in_hex(hermes_recieve_buffer[i], 16);
		}
	}

	if (test_send_ping)
	{
		printf("\n\n=========== test_send_ping ===========\n");
		hermes_send_ping();

		printf("expe: ");
		print_array_in_hex(ping_data, sizeof(ping_data));

		printf("read: ");
		print_array_in_hex(hermes_recieve_buffer[0], sizeof(ping_data));
	}

	if (test_send_echo)
	{
		printf("\n\n=========== test_send_echo ===========\n");

		// variables
		uint8_t test_send_echo_data[] = {0xcc, 0xab, 0x11, 0x12, 0x13, 0x00, 0xFF, 0xaa};

		// code
		hermes_send_echo(test_send_echo_data, sizeof(test_send_echo_data));

		printf("expe: ");
		print_array_in_hex(test_send_echo_data, sizeof(test_send_echo_data));

		printf("read: ");
		print_array_in_hex(&hermes_recieve_buffer[0][2], sizeof(test_send_echo_data));
	}

	if (Test_TCA9554_I2C_write)
	{
		printf("\n\n=========== Testing I2C write with TCA9554 ===========\n");

		// variables
		uint8_t TCA9554_init[] = {TCA9554_REG_CONFIG, 0x00};
		uint8_t TCA9554_off[] = {TCA9554_REG_OUTPUT, 0x00};
		uint8_t TCA9554_on[] = {TCA9554_REG_OUTPUT, 0b00001111};

		// code

		// init
		printf("TCA init \n");
		hermes_send_I2C_write(TCA9554_ADDR, TCA9554_init, 2);

		// set to low
		printf("setting low\n");
		hermes_send_I2C_write(TCA9554_ADDR, TCA9554_off, 2);

		// Set high
		printf("setting high\n");
		hermes_send_I2C_write(TCA9554_ADDR, TCA9554_on, 2);
		delay_ms(2000);

		// set low
		printf("setting low\n");
		hermes_send_I2C_write(TCA9554_ADDR, TCA9554_off, 2);
		delay_ms(2000);

		// Set high
		printf("setting high\n");
		hermes_send_I2C_write(TCA9554_ADDR, TCA9554_on, 2);
		delay_ms(2000);

		// set low
		printf("setting low\n");
		hermes_send_I2C_write(TCA9554_ADDR, TCA9554_off, 2);
		delay_ms(2000);
	}

	if (Test_CH32V003_I2C_write_and_delay)
	{
		printf("\n\n=========== Testing I2C write with TCA9554 and delay ===========\n");

		// variables
		// variables
		uint8_t TCA9554_init_delay[] = {TCA9554_REG_CONFIG, 0x00};
		uint8_t TCA9554_off_delay[] = {TCA9554_REG_OUTPUT, 0x00};
		uint8_t TCA9554_on_delay[] = {TCA9554_REG_OUTPUT, 0b00001111};

		// code

		hermes_add_I2C_write(TCA9554_ADDR, TCA9554_init_delay, 2); // init
		hermes_add_I2C_write(TCA9554_ADDR, TCA9554_off_delay, 2);  // set low
		hermes_add_delay_ms(200);								   // delay 200ms
		hermes_add_I2C_write(TCA9554_ADDR, TCA9554_on_delay, 2);   // set high
		hermes_add_delay_ms(200);								   // delay 200ms
		hermes_add_I2C_write(TCA9554_ADDR, TCA9554_off_delay, 2);  // set low
		hermes_add_delay_ms(200);								   // delay 200ms
		hermes_add_I2C_write(TCA9554_ADDR, TCA9554_on_delay, 2);   // set high
		hermes_add_delay_ms(200);								   // delay 200ms
		hermes_add_I2C_write(TCA9554_ADDR, TCA9554_off_delay, 2);  // set low

		hermes_packet_flush(); // flush stack
	}

	if (Test_hot_plate_send_recieve)
	{

		printf("\n\n=========== Testing I2C send recieve with hotplate ===========\n");

		// Write first set of data
		hermes_send_I2C_write(Hot_plate_address, dummy1_data_write_1, sizeof(dummy1_data_write_1));
		hermes_send_I2C_write(Hot_plate_address, dummy2_data_write_1, sizeof(dummy2_data_write_1));
		hermes_send_I2C_write(Hot_plate_address, dummy3_data_write_1, sizeof(dummy3_data_write_1));

		printf("\n");
		printf("Dummy 1 Write 1: __ __ __ ");
		print_array_in_hex(dummy1_data_write_1, sizeof(dummy1_data_write_1));
		printf("Dummy 2 Write 1: __ __ __ ");
		print_array_in_hex(dummy2_data_write_1, sizeof(dummy2_data_write_1));
		printf("Dummy 3 Write 1: __ __ __ ");
		print_array_in_hex(dummy3_data_write_1, sizeof(dummy3_data_write_1));
		delay_ms(2000);

		// Read first set of data
		hermes_send_I2C_send_recieve(Hot_plate_address, 1, dummy1_len, dummy1_ID_in_uint8);
		destination = dummy1_data_read1;
		source = hermes_recieve_buffer[0];
		length = sizeof(dummy1_data_read1);
		memcpy(destination, source, length);

		hermes_send_I2C_send_recieve(Hot_plate_address, 1, dummy2_len, dummy2_ID_in_uint8);
		destination = dummy2_data_read1;
		source = hermes_recieve_buffer[0];
		length = sizeof(dummy2_data_read1);
		memcpy(destination, source, length);

		hermes_send_I2C_send_recieve(Hot_plate_address, 1, dummy3_len, dummy3_ID_in_uint8);
		destination = dummy3_data_read1;
		source = hermes_recieve_buffer[0];
		length = sizeof(dummy3_data_read1);
		memcpy(destination, source, length);

		printf("\n");
		printf("Dummy 1 Read_ 1: ");
		print_array_in_hex(dummy1_data_read1, sizeof(dummy1_data_read1));
		printf("Dummy 2 Read_ 1: ");
		print_array_in_hex(dummy2_data_read1, sizeof(dummy2_data_read1));
		printf("Dummy 3 Read_ 1: ");
		print_array_in_hex(dummy3_data_read1, sizeof(dummy3_data_read1));

		// Write second set of data
		hermes_send_I2C_write(Hot_plate_address, dummy1_data_write_2, sizeof(dummy1_data_write_2));
		hermes_send_I2C_write(Hot_plate_address, dummy2_data_write_2, sizeof(dummy2_data_write_2));
		hermes_send_I2C_write(Hot_plate_address, dummy3_data_write_2, sizeof(dummy3_data_write_2));

		printf("\n");
		printf("Dummy 1 Write 2: __ __ __ ");
		print_array_in_hex(dummy1_data_write_2, sizeof(dummy1_data_write_1));
		printf("Dummy 2 Write 2: __ __ __ ");
		print_array_in_hex(dummy2_data_write_2, sizeof(dummy2_data_write_2));
		printf("Dummy 3 Write 2: __ __ __ ");
		print_array_in_hex(dummy3_data_write_2, sizeof(dummy3_data_write_2));

		delay_ms(2000);

		// Read second set of data

		hermes_send_I2C_send_recieve(Hot_plate_address, 1, dummy1_len, dummy1_ID_in_uint8);
		destination = dummy1_data_read2;
		source = hermes_recieve_buffer[0];
		length = sizeof(dummy1_data_read2);
		memcpy(destination, source, length);

		hermes_send_I2C_send_recieve(Hot_plate_address, 1, dummy2_len, dummy2_ID_in_uint8);
		destination = dummy2_data_read2;
		source = hermes_recieve_buffer[0];
		length = sizeof(dummy2_data_read2);
		memcpy(destination, source, length);

		hermes_send_I2C_send_recieve(Hot_plate_address, 1, dummy3_len, dummy3_ID_in_uint8);
		destination = dummy3_data_read2;
		source = hermes_recieve_buffer[0];
		length = sizeof(dummy3_data_read2);
		memcpy(destination, source, length);

		printf("\n");
		printf("Dummy 1 Read_ 2: ");
		print_array_in_hex(dummy1_data_read2, sizeof(dummy1_data_read2));
		printf("Dummy 2 Read_ 2: ");
		print_array_in_hex(dummy2_data_read2, sizeof(dummy2_data_read2));
		printf("Dummy 2 Read_ 2: ");
		print_array_in_hex(dummy3_data_read2, sizeof(dummy3_data_read2));

		// check if everything is good
		int current_loop_bad = 0;
		printf("agrigated results:\n");

		// FIX: Start loop at 1 to skip the Command ID byte check
		for (int i = 1; i < 9; i++)
		{
			current_loop_bad = 0;
			printf("Loop %i: ", i);

			if (i <= dummy1_len)
			{
				if (dummy1_data_write_1[i] != dummy1_data_read1[i + 3])
				{
					current_loop_bad = 1;
					printf("D1 W1; ");
				}

				if (dummy1_data_write_2[i] != dummy1_data_read2[i + 3])
				{
					current_loop_bad = 1;
					printf("D1 W2; ");
				}
			}

			if (i <= dummy2_len)
			{
				if (dummy2_data_write_1[i] != dummy2_data_read1[i + 3])
				{
					current_loop_bad = 1;
					printf("D2 W1; ");
				}

				if (dummy2_data_write_2[i] != dummy2_data_read2[i + 3])
				{
					current_loop_bad = 1;
					printf("D2 W2; ");
				}
			}
			if (i <= dummy3_len)
			{
				if (dummy3_data_write_1[i] != dummy3_data_read1[i + 3])
				{
					current_loop_bad = 1;
					printf("D3 W1; ");
				}

				if (dummy3_data_write_2[i] != dummy3_data_read2[i + 3])
				{
					current_loop_bad = 1;
					printf("D3 W2; ");
				}
			}

			if (!current_loop_bad)
			{
				printf("Everything good!");
			}

			printf("\n");
		}
	}

	if (Test_hot_plate_send_and_read)
	{
		printf("\n\n=========== Testing I2C Read and Write with hotplate ===========\n");

		// Write first set of data
		hermes_send_I2C_write(Hot_plate_address, dummy1_data_write_1, sizeof(dummy1_data_write_1));
		hermes_send_I2C_write(Hot_plate_address, dummy2_data_write_1, sizeof(dummy2_data_write_1));
		hermes_send_I2C_write(Hot_plate_address, dummy3_data_write_1, sizeof(dummy3_data_write_1));

		printf("\n");
		printf("Dummy 1 Write 1: __ __ __ ");
		print_array_in_hex(dummy1_data_write_1, sizeof(dummy1_data_write_1));
		printf("Dummy 2 Write 1: __ __ __ ");
		print_array_in_hex(dummy2_data_write_1, sizeof(dummy2_data_write_1));
		printf("Dummy 3 Write 1: __ __ __ ");
		print_array_in_hex(dummy3_data_write_1, sizeof(dummy3_data_write_1));
		delay_ms(2000);

		// Read first set of data
		hermes_send_I2C_write(Hot_plate_address, dummy1_ID_in_uint8, 1);
		hermes_send_I2C_read(Hot_plate_address, dummy1_len);
		destination = dummy1_data_read1;
		source = hermes_recieve_buffer[0];
		length = sizeof(dummy1_data_read1);
		memcpy(destination, source, length);

		hermes_send_I2C_write(Hot_plate_address, dummy2_ID_in_uint8, 1);
		hermes_send_I2C_read(Hot_plate_address, dummy2_len);
		destination = dummy2_data_read1;
		source = hermes_recieve_buffer[0];
		length = sizeof(dummy2_data_read1);
		memcpy(destination, source, length);

		hermes_send_I2C_write(Hot_plate_address, dummy3_ID_in_uint8, 1);
		hermes_send_I2C_read(Hot_plate_address, dummy3_len);
		destination = dummy3_data_read1;
		source = hermes_recieve_buffer[0];
		length = sizeof(dummy3_data_read1);
		memcpy(destination, source, length);

		printf("\n");
		printf("Dummy 1 Read_ 1: ");
		print_array_in_hex(dummy1_data_read1, sizeof(dummy1_data_read1));
		printf("Dummy 2 Read_ 1: ");
		print_array_in_hex(dummy2_data_read1, sizeof(dummy2_data_read1));
		printf("Dummy 3 Read_ 1: ");
		print_array_in_hex(dummy3_data_read1, sizeof(dummy3_data_read1));

		// Write second set of data
		hermes_send_I2C_write(Hot_plate_address, dummy1_data_write_2, sizeof(dummy1_data_write_2));
		hermes_send_I2C_write(Hot_plate_address, dummy2_data_write_2, sizeof(dummy2_data_write_2));
		hermes_send_I2C_write(Hot_plate_address, dummy3_data_write_2, sizeof(dummy3_data_write_2));

		printf("\n");
		printf("Dummy 1 Write 2: __ __ __ ");
		print_array_in_hex(dummy1_data_write_2, sizeof(dummy1_data_write_1));
		printf("Dummy 2 Write 2: __ __ __ ");
		print_array_in_hex(dummy2_data_write_2, sizeof(dummy2_data_write_2));
		printf("Dummy 3 Write 2: __ __ __ ");
		print_array_in_hex(dummy3_data_write_2, sizeof(dummy3_data_write_2));

		delay_ms(2000);

		// Read second set of data
		hermes_send_I2C_write(Hot_plate_address, dummy1_ID_in_uint8, 1);
		hermes_send_I2C_read(Hot_plate_address, dummy1_len);
		destination = dummy1_data_read2;
		source = hermes_recieve_buffer[0];
		length = sizeof(dummy1_data_read2);
		memcpy(destination, source, length);

		hermes_send_I2C_write(Hot_plate_address, dummy2_ID_in_uint8, 1);
		hermes_send_I2C_read(Hot_plate_address, dummy2_len);
		destination = dummy2_data_read2;
		source = hermes_recieve_buffer[0];
		length = sizeof(dummy2_data_read2);
		memcpy(destination, source, length);

		hermes_send_I2C_write(Hot_plate_address, dummy3_ID_in_uint8, 1);
		hermes_send_I2C_read(Hot_plate_address, dummy3_len);
		destination = dummy3_data_read2;
		source = hermes_recieve_buffer[0];
		length = sizeof(dummy3_data_read2);
		memcpy(destination, source, length);

		printf("\n");
		printf("Dummy 1 Read_ 2: ");
		print_array_in_hex(dummy1_data_read2, sizeof(dummy1_data_read2));
		printf("Dummy 2 Read_ 2: ");
		print_array_in_hex(dummy2_data_read2, sizeof(dummy2_data_read2));
		printf("Dummy 2 Read_ 2: ");
		print_array_in_hex(dummy3_data_read2, sizeof(dummy3_data_read2));

		// check if everything is good
		int current_loop_bad = 0;
		printf("agrigated results:\n");

		// FIX: Start loop at 1 to skip the Command ID byte check
		for (int i = 1; i < 9; i++)
		{
			current_loop_bad = 0;
			printf("Loop %i: ", i);

			if (i <= dummy1_len)
			{
				if (dummy1_data_write_1[i] != dummy1_data_read1[i + 3])
				{
					current_loop_bad = 1;
					printf("D1 W1; ");
				}

				if (dummy1_data_write_2[i] != dummy1_data_read2[i + 3])
				{
					current_loop_bad = 1;
					printf("D1 W2; ");
				}
			}

			if (i <= dummy2_len)
			{
				if (dummy2_data_write_1[i] != dummy2_data_read1[i + 3])
				{
					current_loop_bad = 1;
					printf("D2 W1; ");
				}

				if (dummy2_data_write_2[i] != dummy2_data_read2[i + 3])
				{
					current_loop_bad = 1;
					printf("D2 W2; ");
				}
			}
			if (i <= dummy3_len)
			{
				if (dummy3_data_write_1[i] != dummy3_data_read1[i + 3])
				{
					current_loop_bad = 1;
					printf("D3 W1; ");
				}

				if (dummy3_data_write_2[i] != dummy3_data_read2[i + 3])
				{
					current_loop_bad = 1;
					printf("D3 W2; ");
				}
			}

			if (!current_loop_bad)
			{
				printf("Everything good!");
			}

			printf("\n");
		}
	}

	if (CH32V003_bootloader_jump_testing)
	{
		printf("\n\n=========== CH32V003_bootloader_jump_testing ===========\n");
		// reset
		hermes_easy_I2C_reset(Ch32V003_bootloader_jump_testing_addr);

		// jump to boot
		hermes_easy_I2C_jump_to_bootloader(Ch32V003_bootloader_jump_testing_addr);
		delay_ms(3000);

		// reset
		hermes_easy_I2C_reset(Ch32V003_bootloader_jump_testing_boot_addr);

		// reset
		hermes_easy_I2C_reset(Ch32V003_bootloader_jump_testing_addr);
	}

	if (CH32V003_bootloader_get_version_testing)
	{
		printf("\n\n=========== Testing CH32V003 Bootloader get version  ===========\n");

		// variables

		uint8_t bootloader_get_data_to_read_amount = 8;
		uint8_t bootloader_get_version_command[] = {Command_ID_I2C_Slave_Flash_Get_Version};

		// code
		hermes_easy_I2C_jump_to_bootloader(CH32V003_bootloader_get_version_testing_addr);
		hermes_send_I2C_write(CH32V003_bootloader_get_version_testing_addr, bootloader_get_version_command, 1);
		delay_ms(50);
		hermes_send_I2C_send_recieve(CH32V003_bootloader_get_version_testing_addr, 1, bootloader_get_data_to_read_amount, bootloader_get_version_command);
		printf("Bootloader version:\n");
		print_array_in_hex(hermes_recieve_buffer[0], 8);
		printf("\n");
		hermes_easy_I2C_reset(CH32V003_bootloader_get_version_testing_addr);
	}

	if (CH32V003_FLASH_read_testing)
	{
		printf("\n\n=========== Testing CH32V003 Bootloader Flash Reading ===========\n");

		// Variables

		uint32_t Flash_pointer = Flash_Start;

		// Write Packets
		uint8_t Flash_read_Command_ID_I2C_Slave_Flash_Set_Pointer[] = {Command_ID_I2C_Slave_Flash_Set_Pointer};
		uint8_t Flash_read_write_flash_pointer_command[] = {Command_ID_I2C_Slave_Flash_Set_Pointer, 0, 0, 0, 0};
		uint8_t Flash_read_ask_for_read[] = {Command_ID_I2C_Slave_Flash_Read_Page};

		// 1. Jump to Bootloader
		printf("Reseting...\n");
		hermes_easy_I2C_reset(CH32V003_FLASH_read_testing_addr);
		printf("Jumping to bootloader...\n");
		hermes_easy_I2C_jump_to_bootloader(CH32V003_FLASH_read_testing_addr);
		delay_ms(50);

		for (int i = 0; i < FLASH_PAGE_AMOUNT; i++)
		{
			// Set flash pointer
			Flash_read_write_flash_pointer_command[1] = Flash_pointer & 0xFF; // low byte
			Flash_read_write_flash_pointer_command[2] = (Flash_pointer >> 8) & 0xFF;
			Flash_read_write_flash_pointer_command[3] = (Flash_pointer >> 16) & 0xFF;
			Flash_read_write_flash_pointer_command[4] = (Flash_pointer >> 24) & 0xFF; // high byte

			// send the flash pointer
			if (CH32V003_FLASH_read_testing_verbose)
			{
				printf("Writing the flash pointer...\n");
				printf("sent flash pointer is:\n");
				print_array_in_hex(&Flash_read_write_flash_pointer_command[2], 4);
			}
			hermes_send_I2C_write(CH32V003_FLASH_read_testing_addr, Flash_read_write_flash_pointer_command, 5);

			// read flash pointer for debbuging
			if (CH32V003_FLASH_read_testing_verbose)
			{
				printf("read flash pointer is:\n");
				hermes_send_I2C_send_recieve(CH32V003_FLASH_read_testing_addr, 1, 4, Flash_read_Command_ID_I2C_Slave_Flash_Set_Pointer);
				print_array_in_hex(&hermes_recieve_buffer[0][4], 4);
			}

			// Ask for a read
			if (CH32V003_FLASH_read_testing_verbose)
			{
				printf("Asking for a FLASH read ...\n");
			}
			hermes_send_I2C_write(CH32V003_FLASH_read_testing_addr, Flash_read_ask_for_read, 1);
			delay_ms(1);

			// read the data
			if (CH32V003_FLASH_read_testing_verbose)
			{
				printf("Reading data...\n");
			}
			hermes_send_I2C_send_recieve(CH32V003_FLASH_read_testing_addr, 1, FLASH_READ_SIZE, Flash_read_ask_for_read);

			// display the data
			// Loop through the 64 bytes (FLASH_READ_SIZE), 16 bytes at a time
			for (int row = 0; row < FLASH_READ_SIZE; row += 16)
			{
				// 1. Print the Flash Pointer (Address) for this line
				// e.g., 08000000, 08000010, 08000020...
				printf("%08X: ", (unsigned int)(Flash_pointer + row));

				// 2. Print the 16 bytes of Flash data from the buffer
				for (int col = 0; col < 16; col++)
				{
					// Access the byte at [row + col]
					printf("%02X ", hermes_recieve_buffer[0][row + col + 4]);
				}

				// 3. New line to move to the next row (e.g. from ...00 to ...10)
				printf("\n");
			}

			// advance flash pointer
			Flash_pointer = Flash_pointer + FLASH_READ_SIZE;
		}

		printf("\nFinished reading!\n");
		printf("Reseting...\n");
		hermes_easy_I2C_reset(CH32V003_FLASH_read_testing_addr);
	}

	if (CH32V003_FLASH_easy_read_testing)
	{
		printf("\n\n=========== CH32V003_FLASH_easy_read_testing ===========\n");

		// variables
		uint32_t easy_read_flash_pointer = Flash_Start;

		// code

		printf("Reseting...\n");
		hermes_easy_I2C_reset(CH32V003_FLASH_easy_read_testing_addr);

		printf("Jumping to bootloader...\n");
		hermes_easy_I2C_jump_to_bootloader(CH32V003_FLASH_easy_read_testing_addr);

		printf("writing flash pointer: %02X\n", Flash_Start);
		hermes_easy_I2C_add_send_flash_pointer(CH32V003_FLASH_easy_read_testing_addr, Flash_Start);

		for (int i = 0; i < FLASH_PAGE_AMOUNT / CH32V003_FLASH_easy_read_testing_reads_in_packet; i++)
		{

			// add the reads
			for (int i = 0; i < CH32V003_FLASH_easy_read_testing_reads_in_packet; i++)
			{
				hermes_easy_I2C_add_read_flash(CH32V003_FLASH_easy_read_testing_addr, FLASH_READ_SIZE);
			}

			// execute the reads
			hermes_packet_flush();

			// parse the reads
			// 1. Only ONE loop to iterate through the packets we actually received
			for (int j = 0; j < CH32V003_FLASH_easy_read_testing_reads_in_packet; j++)
			{
				// 2. Each packet is FLASH_READ_SIZE (64 bytes).
				// We split those 64 bytes into rows of 16 bytes for a pretty dump.
				for (int row = 0; row < FLASH_READ_SIZE; row += 16)
				{
					// Print the Address for this specific line
					printf("%08X: ", (unsigned int)(easy_read_flash_pointer + row));

					// Print 16 bytes of data
					for (int col = 0; col < 16; col++)
					{
						// [j] is the packet index
						// [row + col + 4] is the byte inside that packet
						// (+4 skips the Hermes/I2C header bytes)
						printf("%02X ", hermes_recieve_buffer[j][row + col + 4]);
					}

					// Move to the next line in the terminal
					printf("\n");
				}

				// 3. IMPORTANT: After printing all rows for ONE packet,
				// advance the pointer by 64 so the next packet starts at the right address.
				easy_read_flash_pointer += FLASH_READ_SIZE;
			}
		}

		printf("Reseting\n");
		hermes_easy_I2C_reset(CH32V003_FLASH_easy_read_testing_addr);
		printf("Done\n");
	}

	if (CH32V003_FLASH_WRITE_testing)
	{
		printf("\n\n=========== CH32V003_FLASH_WRITE_testing ===========\n");

		// variables

		// code

		// send the flash pointer
		// read

		// send the flash pointer
		// write
		// wait
		// send the flash pointer
		// read

		// send the flash pointer
		// write
		// wait
		// send the flash pointer
		// read
	}

	// Cleanup
	printf("\n\n\n");
	hermes_USB_deinit();
	printf("\n--- Test Complete ---\n");
	helper_end_timer(1);

	// 8. wait for user input
	// printf("Press any key to exit...\n");
	// getch();

	return 0;
}
