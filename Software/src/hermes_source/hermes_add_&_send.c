#include "hermes_header.h"
#include "Command_ID.h"
#include <string.h>

int hermes_add_ping(void)
{
	uint8_t ping_packet[] = {2, Command_ID_USB_Device_Ping};

	if (HERMES_VERBOSITY_USB > 1)
	{
		printf("-LOG- VERBOSE PACKET: Adding ping to stack\n");
	}

	return hermes_packet_add_comand(ping_packet, sizeof(ping_packet));
}

int hermes_add_echo(uint8_t *data, uint8_t len)
{
	uint8_t echo_packet[HERMES_BUFFER_SEND_MAX_LENGTH] = {};

	echo_packet[0] = len + 2;
	echo_packet[1] = Command_ID_USB_Device_Echo;

	uint8_t *source = data;
	uint8_t *destination = &echo_packet[2];
	int length = len;

	memcpy(destination, source, length);

	if (HERMES_VERBOSITY_USB > 1)
	{
		printf("-LOG- VERBOSE PACKET: Adding echo to stack\n");
		for (int i = 0; i < len + 2; i++)
		{
			printf(" %02X", echo_packet[i]);
		}
		printf("\n");
	}

	return hermes_packet_add_comand(echo_packet, len + 2);
}

int hermes_send_ping(void)
{
	bool error_code = 0;

	if (hermes_packet_check_heigh_taken() == 0)
	{
		error_code = hermes_add_ping();

		if (error_code < 0)
		{
			/* code */
		}

		hermes_packet_flush();
	}
	else
	{

		printf("ERROR PACKET, send_ping: There is already a command in stack\n");
		return -1;
	}
}