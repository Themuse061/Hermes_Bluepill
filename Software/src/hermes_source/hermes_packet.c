#include "hermes_header.h"
#include "string.h"

// -------------------- defining externs from hermes_header.h -------------------- //

uint8_t hermes_send_buffer[HERMES_BUFFER_SEND_STACK_MAX_HEIGHT][HERMES_BUFFER_SEND_MAX_LENGTH]; // One is added for ping on flush
uint8_t hermes_recieve_buffer[HERMES_BUFFER_RECIEVE_STACK_MAX_HEIGHT][HERMES_BUFFER_RECIEVE_MAX_LENGTH];

// -------------------- defining local variables -------------------- //
int hermes_current_send_stack_position = 0;
uint8_t hermes_temp_buffer_bytes[HERMES_BUFFER_MAX_BYTES];
static const uint8_t ping_data[] = {0x09, 0x04, 0xFF, 0xaa, 0x00, 0x11, 0x00, 0xaa, 0xFF};

// ------------------------------------------------------------------------------------------------ //
// ---------------------------------------- FUNCTION START ---------------------------------------- //
// ------------------------------------------------------------------------------------------------ //

// -------------------- Local functions -------------------- //
static void hermes_packet_advance_stack_position()
{
	hermes_current_send_stack_position++;
	if (HERMES_PACKET_VERBOSITY > 1)
	{
		printf("-LOG- VERBOSE PACKET, hermes_packet_advance_stack_position: Advanced stack position to %i\n", hermes_current_send_stack_position);
	}
}

static void log_packet(const uint8_t *data, uint8_t len)
{
	for (int i = 0; i < len; i++)
	{
		printf(" %02X", data[i]);
	}
}

// -------------------- Global functions -------------------- //

int hermes_packet_check_height_free()
{
	int free_spaces = HERMES_BUFFER_SEND_STACK_MAX_HEIGHT - hermes_current_send_stack_position;
	if (HERMES_PACKET_VERBOSITY > 1)
	{
		printf("-LOG- VERBOSE PACKET, hermes_packet_check_height_free: There are %i free spaces\n", free_spaces);
	}
	return free_spaces;
}

int hermes_packet_check_heigh_taken()
{
	if (HERMES_PACKET_VERBOSITY > 1)
	{
		printf("-LOG- VERBOSE PACKET, hermes_packet_check_heigh_taken: There are %i taken spaces\n", hermes_current_send_stack_position);
	}
	return hermes_current_send_stack_position;
}

int hermes_packet_add_comand(uint8_t *command_array, int len)
{

	// check if there is free space for a command
	if (hermes_packet_check_height_free())
	{
		// Check if the command is too long
		if (len < HERMES_BUFFER_SEND_MAX_LENGTH)
		{
			if (HERMES_PACKET_VERBOSITY > 1)
			{
				printf("-LOG- VERBOSE PACKET, hermes_packet_add_comand: Added a packet at height %i:", hermes_current_send_stack_position);
				log_packet(command_array, len);
				printf("\n");
			}

			// make sure that the place for command is empty
			for (int i = 0; i < HERMES_BUFFER_SEND_MAX_LENGTH; i++)
			{
				hermes_send_buffer[hermes_current_send_stack_position][i] = 0;
			}

			// copy the command
			uint8_t *source = command_array;
			uint8_t *destination = hermes_send_buffer[hermes_current_send_stack_position];
			int length = len;

			memcpy(destination, source, length);
			hermes_packet_advance_stack_position();
			return 1;
		}
		else
		{
			printf("ERROR PACKET, hermes_packet_add_comand: Command is too long (%i bytes)\n", len);
			return -1;
		}
	}
	else
	{
		printf("ERROR PACKET, hermes_packet_add_comand: No free space to add a packet\n");
		return -1;
	}
}

int hermes_packet_flush_blind()
{

	// Print raw data from send buffer
	// if (HERMES_PACKET_VERBOSITY > 1)
	// {
	// 	printf("-LOG- VERBOSE PACKET, hermes_packet_flush_blind: Data in hermes_send_buffer (cut to 32 bytes):\n");
	// 	for (int i = 0; i < HERMES_BUFFER_SEND_STACK_MAX_HEIGHT; i++)
	// 	{

	// 		printf("%i: ", i);
	// 		for (int j = 0; j < 32; j++)
	// 		{

	// 			printf("%02X ", hermes_send_buffer[i][j]);
	// 		}
	// 		printf("\n");
	// 	}
	// }

	int current_processed_command = 0;
	int bytes_in_buffer = 0;

	// reset temp buffer
	memset(hermes_temp_buffer_bytes, 0, sizeof(hermes_temp_buffer_bytes));

	while (hermes_send_buffer[current_processed_command][0] && current_processed_command < HERMES_BUFFER_SEND_STACK_MAX_HEIGHT)
	{
		uint8_t *source = hermes_send_buffer[current_processed_command];
		uint8_t *destination = &hermes_temp_buffer_bytes[bytes_in_buffer];
		int length = hermes_send_buffer[current_processed_command][0];

		memcpy(destination, source, length);
		bytes_in_buffer += length;
		current_processed_command++;

		if (bytes_in_buffer > HERMES_BUFFER_MAX_BYTES)
		{
			printf("ERROR PACKET, hermes_packet_flush_blind: hermes_temp_buffer_bytes overflow\n");
		}
	}

	int bytes_send = hermes_USB_send((char *)hermes_temp_buffer_bytes, bytes_in_buffer);

	if (bytes_send == bytes_in_buffer)
	{
		if (HERMES_PACKET_VERBOSITY > 1)
		{
			printf("-LOG- VERBOSE PACKET, hermes_packet_flush_blind: Sucessfull:\n");
		}
		// reset stack
		hermes_current_send_stack_position = 0;

		// reset send buffer
		memset(hermes_send_buffer, 0, sizeof(hermes_send_buffer));

		return bytes_send;
	}
	else
	{
		if (bytes_send < 0)
		{
			printf("ERROR PACKET, hermes_packet_flush_blind: USB write error\n");
			return bytes_send;
		}

		printf("ERROR PACKET, hermes_packet_flush_blind: We have send less bytes than we should\n");
		return bytes_send;
	}
}

/// @brief Read USB and puts it into buffer
/// @param new_data_place where to add new data. will override new_data_place[0]
/// @return how many bytes it read
static int hermes_read_usb_and_put_it_into_some_buffer(uint8_t *new_data_place)
{
	int data_in_buffer_amount = hermes_USB_check_recieve_buffer();

	if (data_in_buffer_amount)
	{

		if (HERMES_PACKET_VERBOSITY > 1)
		{
			printf("-LOG- VERBOSE PACKET, hermes_read_usb_and_put_it_into_some_buffer: there are %i bytes in buffer\n", data_in_buffer_amount);
			printf("-LOG- VERBOSE PACKET, hermes_read_usb_and_put_it_into_some_buffer: copying data to some buffer\n");
		}
		hermes_USB_recieve(new_data_place, data_in_buffer_amount);
	}
	return data_in_buffer_amount;
}

int hermes_packet_flush()
{
	uint8_t proprietary_echo_data[] = {0x15, 0x17, 0xaa, 0xcc, 0x28, 0x01, 0x10, 0xFF};

	int return_code = hermes_add_echo(proprietary_echo_data, sizeof(proprietary_echo_data));
	if (return_code < 0)
	{
		printf("ERROR PACKET, hermes_packet_flush: hermes_add_ping threw an error\n");
		return return_code;
	}

	return_code = hermes_packet_flush_blind();

	if (return_code < 0)
	{
		printf("ERROR PACKET, hermes_packet_flush: hermes_packet_flush_blind threw an error\n");
		return return_code;
	}

	// reset recieve buffer
	memset(hermes_recieve_buffer, 0, sizeof(hermes_recieve_buffer));

	int current_command = 0;
	int new_bytes_amount = 0;
	bool can_exit_do_loop = 0;
	int loops = 0;
	do
	{
		// reset temp buffer
		memset(hermes_temp_buffer_bytes, 0, sizeof(hermes_temp_buffer_bytes));

		// read usb to temp buffer
		new_bytes_amount = hermes_read_usb_and_put_it_into_some_buffer(hermes_temp_buffer_bytes);

		// parse new data
		int buffer_ptr = 0;
		uint8_t *source;
		uint8_t *destination;
		int length;

		while (hermes_temp_buffer_bytes[buffer_ptr])
		{
			source = &hermes_temp_buffer_bytes[buffer_ptr];
			destination = &hermes_recieve_buffer[current_command][0];
			length = hermes_temp_buffer_bytes[buffer_ptr];
			if (HERMES_PACKET_VERBOSITY > 1)
			{
				printf("-LOG- VERBOSE PACKET, hermes_packet_parse_USB: Copying len %i from %i to command nr %i\n", length, buffer_ptr, current_command);
			}

			memcpy(destination, source, length);
			buffer_ptr += hermes_temp_buffer_bytes[buffer_ptr];
			current_command++;
		}

		// check if last command is echo
		if (memcmp(&hermes_recieve_buffer[current_command - 1][2], proprietary_echo_data, sizeof(proprietary_echo_data)) == 0)
		{
			can_exit_do_loop = 1;
		}

		loops++;

	} while (!can_exit_do_loop);

	// remove echo from buffer
	for (int i = 0; i < HERMES_BUFFER_RECIEVE_MAX_LENGTH; i++)
	{
		hermes_recieve_buffer[current_command - 1][i] = 0;
	}
	if (HERMES_PACKET_VERBOSITY > 1)
	{
		printf("-LOG- VERBOSE PACKET, hermes_packet_flush: there were %i USB reading loops\n", loops);
	}

	return 1;
}

int hermes_packet_parse_USB()
{

	if (HERMES_PACKET_VERBOSITY > 1)
	{
		printf("-LOG- VERBOSE PACKET, hermes_packet_parse_USB: Parsing the buffer\n");
	}

	// reset temp buffer
	memset(hermes_temp_buffer_bytes, 0, sizeof(hermes_temp_buffer_bytes));

	// Read da data
	int read_data_amount = hermes_read_usb_and_put_it_into_some_buffer(hermes_temp_buffer_bytes);

	if (read_data_amount)
	{
		// reset recieve buffer
		memset(hermes_recieve_buffer, 0, sizeof(hermes_USB_recieve));

		// Parse that data
		int buffer_ptr = 0;
		int current_command = 0;
		uint8_t *source;
		uint8_t *destination;
		int length;

		while (hermes_temp_buffer_bytes[buffer_ptr])
		{
			source = &hermes_temp_buffer_bytes[buffer_ptr];
			destination = &hermes_recieve_buffer[current_command][0];
			length = hermes_temp_buffer_bytes[buffer_ptr];
			if (HERMES_PACKET_VERBOSITY > 1)
			{
				printf("-LOG- VERBOSE PACKET, hermes_packet_parse_USB: Copying len %i from %i to command nr %i\n", length, buffer_ptr, current_command);
			}

			memcpy(destination, source, length);
			buffer_ptr += hermes_temp_buffer_bytes[buffer_ptr];
			current_command++;
		}
		if (HERMES_PACKET_VERBOSITY > 1)
		{
			printf("-LOG- VERBOSE PACKET, hermes_packet_parse_USB: Parsing done\n");
		}
		return 1;
	}
	else if (read_data_amount == 0)
	{
		printf("ERROR PACKET, hermes_packet_parse_USB: Read 0 bytes\n");
		return -1;
	}
	else
	{
		printf("ERROR PACKET, hermes_packet_parse_USB: Error reading from USB\n");
		return -1;
	}
}