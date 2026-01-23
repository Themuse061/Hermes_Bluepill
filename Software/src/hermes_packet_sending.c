#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Usb_driver.h"
#include "Command_ID.h"
#include "hermes_packet_sending.h"

#define HERMES_VERBOSE_LEVEL 2 // 0 for quiet, 1 for normal, 2 for verbose
#define HERMES_MAX_TIMEOUT 5000

#define HERMES_MAX_STACK_HEIGHT 16
#define HERMES_MAX_PACKET_WIDTH 64
#define HERMES_TOTAL_BUFFER_SIZE (HERMES_MAX_STACK_HEIGHT * HERMES_MAX_PACKET_WIDTH)

// Global state
static uint8_t hermes_packet_stack[HERMES_TOTAL_BUFFER_SIZE];
static int hermes_packet_stack_current_len = 0;
static int hermes_packet_stack_height = 0;

static void log_packet(const uint8_t *data, uint8_t len)
{
	for (int i = 0; i < len; i++)
	{
		printf(" %02X", data[i]);
	}
}

int Hermes_Add_Command_To_Stack(uint8_t *new_data, uint8_t len)
{
	// check if the data is empty
	if (new_data == NULL || len == 0)
	{
		printf("Hermes_Add_Command_To_Stack Failed, -1 Null data or len 0\n");
		return -1;
	}

	// check if data is too long for a single packet slot
	// (This check is less relevant if we are just streaming bytes, but strict packet widths might be enforced)
	if (len > HERMES_MAX_PACKET_WIDTH)
	{
		printf("Hermes_Add_Command_To_Stack Failed, -2 Data too long\n");
		return -2;
	}

	// Check total buffer overflow
	if ((hermes_packet_stack_current_len + len) > HERMES_TOTAL_BUFFER_SIZE)
	{
		printf("Hermes_Add_Command_To_Stack Failed, -3 Buffer full\n");
		return -3; // Buffer full
	}

	// Check packet count limit
	if (hermes_packet_stack_height >= HERMES_MAX_STACK_HEIGHT)
	{
		printf("Hermes_Add_Command_To_Stack Failed, -4 Stack full\n");
		return -4; // Too many packets
	}

	// Copy the data
	memcpy(&hermes_packet_stack[hermes_packet_stack_current_len], new_data, len);
	hermes_packet_stack_current_len += len;
	hermes_packet_stack_height++;

	if (HERMES_VERBOSE_LEVEL > 1)
	{
		printf("added packet to stack: ");
		log_packet(new_data, len);
		printf("\n");
		printf("Hermes_Add_Command_To_Stack Successful\n");
	}

	return 1;
}

int Hermes_Check_Stack_Height(void)
{
	return hermes_packet_stack_height;
}

int Hermes_Flush_Stack(void)
{
	if (hermes_packet_stack_current_len == 0)
		return 0;

	int len_to_send = hermes_packet_stack_current_len;

	// Reset counters
	hermes_packet_stack_current_len = 0;
	hermes_packet_stack_height = 0;

	// Send the data
	if (HERMES_VERBOSE_LEVEL > 0)
	{
		printf("flushing stack: ");
		log_packet(hermes_packet_stack, len_to_send);
		printf("\n");
	}

	int sent = USB_write(hermes_packet_stack, len_to_send);
	if (sent <= 0)
	{
		printf("Hermes_Flush_Stack Failed, USB_write error: %d\n", sent);
	}
	else
	{
		if (HERMES_VERBOSE_LEVEL > 1)
		{
			printf("Hermes_Flush_Stack Successful, sent %d bytes\n", sent);
		}
	}
	return sent;
}

int Hermes_Flush_Stack_with_wait(void)
{
	// Corrected initialization: array of bytes, not string literal
	uint8_t echo_packet[] = {0x05, Command_ID_USB_Device_Echo, 0xAC, 0xCA, 0xFD};
	uint8_t echo_packet_return[sizeof(echo_packet)];

	// 1. Add echo to the END of the current stack
	int add_res = Hermes_Add_Command_To_Stack(echo_packet, sizeof(echo_packet));
	if (add_res < 0)
		return add_res;

	// 2. Send the whole stack
	if (HERMES_VERBOSE_LEVEL > 0)
	{
		printf("flushing stack with wait: ");
		log_packet(hermes_packet_stack, hermes_packet_stack_current_len);
		printf("\n");
	}

	int sent = Hermes_Flush_Stack();

	if (sent <= 0)
	{
		printf("Hermes_Flush_Stack_with_wait Failed, -5 Write failed\n");
		return -5; // Write failed
	}

	// 3. Wait for the specific echo response
	// NOTE: This assumes previous commands in the stack do NOT return data.
	// If they do, this read might consume that data instead of the echo.
	int read_len = USB_read(echo_packet_return, sizeof(echo_packet), HERMES_MAX_TIMEOUT);
	if (HERMES_VERBOSE_LEVEL > 1)
	{
		printf("data recieved from flush with wait: ");
		log_packet(echo_packet_return, sizeof(echo_packet));
		printf("\n");
	}

	if (read_len == sizeof(echo_packet))
	{
		// check if return packets are the same
		if (memcmp(echo_packet_return, echo_packet, sizeof(echo_packet)) != 0)
		{
			printf("Hermes_Flush_Stack_with_wait Failed, -1 Mismatch\n");
			return -1; // Mismatch
		}

		// make sure no new data has been send to buffer (rouge read)
		if (USB_check_for_data() != 0)
		{
			printf("Hermes_Flush_Stack_with_wait Failed, -2 Rogue read data present\n");
			return -2;
		}

		if (HERMES_VERBOSE_LEVEL > 1)
		{
			printf("Hermes_Flush_Stack_with_wait Successful\n");
		}
		return 1; // Success
	}
	else
	{
		printf("Hermes_Flush_Stack_with_wait Failed, -4 Timeout or size mismatch\n");
		return -4; // Timeout or size mismatch
	}
}

int Hermes_Flush_Stack_with_Read(uint8_t *read_data_buffer, uint8_t len)
{
	if (HERMES_VERBOSE_LEVEL > 0)
	{
		printf("flushing stack with read: ");
		log_packet(hermes_packet_stack, hermes_packet_stack_current_len);
		printf("\n");
	}
	int sent = Hermes_Flush_Stack();
	if (sent <= 0)
	{
		printf("Hermes_Flush_Stack_with_Read Failed, -5 Write failed\n");
		return -5; // Write failed
	}

	int read_len = USB_read(read_data_buffer, len, HERMES_MAX_TIMEOUT);

	if (HERMES_VERBOSE_LEVEL > 1)
	{
		printf("data recieved from flush with read: ");
		log_packet(read_data_buffer, len);
		printf("\n");
	}

	if (read_len == len)
	{
		if (HERMES_VERBOSE_LEVEL > 1)
		{
			printf("Hermes_Flush_Stack_with_Read Sucesfull\n");
		}
		return 1; // Success
	}
	else
	{
		printf("Hermes_Flush_Stack_with_Read Failed, -4 Timeout or size mismatch\n");
		return -4; // Timeout or size mismatch
	}
}