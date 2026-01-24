#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Usb_driver.h"
#include "Command_ID.h"
#include "hermes_packet_sending.h"

#define HERMES_VERBOSE_LEVEL 1 // 0 for quiet, 1 for normal, 2 for verbose
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

int Hermes_Check_Stack_Height(void)
{
	return hermes_packet_stack_height;
}

int Hermes_Check_Stack_Length(void)
{
	return hermes_packet_stack_current_len;
}

int Hermes_Add_Command_To_Stack(uint8_t *new_data, uint8_t len)
{
	// check if the data is empty
	if (new_data == NULL || len == 0)
	{
		printf("LOG 0: Hermes_Add_Command_To_Stack Failed, -1 Null data or len 0\n");
		return -1;
	}

	// check if data is too long for a single packet slot
	// (This check is less relevant if we are just streaming bytes, but strict packet widths might be enforced)
	if (len > HERMES_MAX_PACKET_WIDTH)
	{
		printf("LOG 0: Hermes_Add_Command_To_Stack Failed, -2 Data too long\n");
		return -2;
	}

	// Check total buffer overflow
	if ((hermes_packet_stack_current_len + len) > HERMES_TOTAL_BUFFER_SIZE)
	{
		printf("LOG 0: Hermes_Add_Command_To_Stack Failed, -3 Buffer full\n");
		return -3; // Buffer full
	}

	// Check packet count limit
	if (hermes_packet_stack_height >= HERMES_MAX_STACK_HEIGHT)
	{
		printf("LOG 0 : Hermes_Add_Command_To_Stack Failed, -4 Stack full\n");
		return -4; // Too many packets
	}

	// Copy the data
	memcpy(&hermes_packet_stack[hermes_packet_stack_current_len], new_data, len);
	hermes_packet_stack_current_len += len;
	hermes_packet_stack_height++;

	if (HERMES_VERBOSE_LEVEL > 1)
	{
		printf("LOG 2: added packet to stack: ");
		log_packet(new_data, len);
		printf("\n");
	}

	return 1;
}

int Hermes_Read_Buffer_USB(uint8_t *read_pointer, int len)
{
	return USB_read(read_pointer, len, HERMES_MAX_TIMEOUT);
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
		printf("LOG 1: flushing stack: ");
		log_packet(hermes_packet_stack, len_to_send);
		printf("\n");
	}

	int sent = USB_write(hermes_packet_stack, len_to_send);
	if (sent <= 0)
	{
		printf("LOG 0: Hermes_Flush_Stack Failed, USB_write error: %d\n", sent);
	}
	else
	{
		if (HERMES_VERBOSE_LEVEL > 1)
		{
			printf("LOG 2: Hermes_Flush_Stack Successful, sent %d bytes\n", sent);
		}
	}
	return sent;
}

int Hermes_Flush_Stack_with_Read(uint8_t *read_data_buffer, uint8_t len)
{
	int Flush_Result = Hermes_Flush_Stack();
	if (Flush_Result)
	{
		return Hermes_Read_Buffer_USB(read_data_buffer, len);
	}
	else
	{
		return Flush_Result;
	}
}

int Hermes_Flush_Stack_with_wait(void)
{
}