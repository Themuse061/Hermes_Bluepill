#include "hermes_header.h"
#include "Command_ID.h"
#include <string.h>

int hermes_add_ping(void)
{
	uint8_t ping_packet[] = {2, Command_ID_USB_Device_Ping};

	if (HERMES_SEND_VERBOSITY > 1)
	{
		printf("-LOG- VERBOSE SEND, hermes_add_ping: Adding ping to stack\n");
	}

	return hermes_packet_add_comand(ping_packet, sizeof(ping_packet));
}

int hermes_send_ping(void)
{
	bool error_code = 0;

	if (hermes_packet_check_heigh_taken() == 0)
	{
		error_code = hermes_add_ping();

		if (error_code < 0)
		{
			printf("-LOG- ERROR SEND, send_ping: Error in adding command\n");
		}

		hermes_packet_flush();
	}
	else
	{

		printf("-LOG- ERROR SEND, send_ping: There is already a command in stack\n");
		return -1;
	}
}

int hermes_add_echo(uint8_t *data, uint8_t len)
{
	uint8_t echo_header[] = {len + 2, Command_ID_USB_Device_Echo};

	hermes_packet_add_comand_without_advancing_stack_height(echo_header, 2);

	if (HERMES_SEND_VERBOSITY > 1)
	{
		printf("-LOG- VERBOSE SEND: Adding echo to stack\n");
		for (int i = 0; i < len + 2; i++)
		{
			printf(" %02X", data[i]);
		}
		printf("\n");
	}

	return hermes_packet_add_comand(data, len);
}

int hermes_send_echo(uint8_t *data, uint8_t len)
{
	bool error_code = 0;

	if (hermes_packet_check_heigh_taken() == 0)
	{
		error_code = hermes_add_echo(data, len);

		if (error_code < 0)
		{
			printf("-LOG- ERROR SEND, hermes_send_echo: Error in adding command\n");
		}

		hermes_packet_flush();
	}
	else
	{

		printf("-LOG- ERROR SEND, hermes_send_echo: There is already a command in stack\n");
		return -1;
	}
}

int hermes_add_I2C_write(uint8_t I2C_address, uint8_t *data, uint8_t len)
{

	uint8_t I2C_write_header[] = {len + 3, Command_ID_USB_Device_I2C_Write, I2C_address};

	hermes_packet_add_comand_without_advancing_stack_height(I2C_write_header, 3);

	if (HERMES_SEND_VERBOSITY > 1)
	{
		printf("-LOG- VERBOSE SEND: Adding I2C write to stack\n-LOG- ");
		printf(" %02X", I2C_address);
		for (int i = 0; i < len + 2; i++)
		{
			printf(" %02X", data[i]);
		}
		printf("\n");
	}

	return hermes_packet_add_comand(data, len);
}

int hermes_send_I2C_write(uint8_t I2C_address, uint8_t *data, uint8_t len)
{
	bool error_code = 0;

	if (hermes_packet_check_heigh_taken() == 0)
	{
		error_code = hermes_add_I2C_write(I2C_address, data, len);

		if (error_code < 0)
		{
			printf("-LOG- ERROR SEND, hermes_send_I2C_write: Error in adding command\n");
		}

		hermes_packet_flush();
	}
	else
	{

		printf("-LOG- ERROR SEND, hermes_send_I2C_write: There is already a command in stack\n");
		return -1;
	}
}

int hermes_add_delay_ms(int delay)
{
	uint8_t add_delay_packet[] = {
		6,								// Total packet length
		Command_ID_USB_Device_Delay_Ms, // Command ID
		(uint8_t)(delay & 0xFF),		// LSB
		(uint8_t)((delay >> 8) & 0xFF),
		(uint8_t)((delay >> 16) & 0xFF),
		(uint8_t)((delay >> 24) & 0xFF) // MSB
	};

	if (HERMES_SEND_VERBOSITY > 1)
	{
		printf("-LOG- VERBOSE SEND, hermes_add_delay_ms: Adding delay %ims to stack\n", delay);
	}

	return hermes_packet_add_comand(add_delay_packet, sizeof(add_delay_packet));
}

int hermes_send_delay_ms(int delay)
{
	bool error_code = 0;

	if (hermes_packet_check_heigh_taken() == 0)
	{
		error_code = hermes_add_delay_ms(delay);

		if (error_code < 0)
		{
			printf("-LOG- ERROR SEND, hermes_send_delay_ms: Error in adding command\n");
		}

		hermes_packet_flush();
	}
	else
	{

		printf("-LOG- ERROR SEND, hermes_send_delay_ms: There is already a command in stack\n");
		return -1;
	}
}