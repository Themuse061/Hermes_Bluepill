#include "hermes_header.h"
#include "Command_ID.h"
#include <string.h>

// ------------------------------      ------------------------------
// ------------------------------ ping ------------------------------
//------------------------------       ------------------------------

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

		return hermes_packet_flush();
	}
	else
	{

		printf("-LOG- ERROR SEND, send_ping: There is already a command in stack\n");
		return -1;
	}
}

// ------------------------------      ------------------------------
// ------------------------------ echo ------------------------------
//------------------------------       ------------------------------

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

		return hermes_packet_flush();
	}
	else
	{

		printf("-LOG- ERROR SEND, hermes_send_echo: There is already a command in stack\n");
		return -1;
	}
}

// ------------------------------      ------------------------------
// ------------------------------ I2C write ------------------------------
//------------------------------       ------------------------------

int hermes_add_I2C_write(uint8_t I2C_address, uint8_t *data, uint8_t len)
{

	uint8_t I2C_write_header[] = {len + 3, Command_ID_USB_Device_I2C_Write, I2C_address};

	hermes_packet_add_comand_without_advancing_stack_height(I2C_write_header, 3);

	if (HERMES_SEND_VERBOSITY > 1)
	{
		printf("-LOG- VERBOSE SEND, hermes_add_I2C_write: Adding I2C write to stack\n-LOG- ");
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

		return hermes_packet_flush();
	}
	else
	{

		printf("-LOG- ERROR SEND, hermes_send_I2C_write: There is already a command in stack\n");
		return -1;
	}
}

// ------------------------------      ------------------------------
// ------------------------------ delay ms ------------------------------
//------------------------------       ------------------------------

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

		return hermes_packet_flush();
	}
	else
	{

		printf("-LOG- ERROR SEND, hermes_send_delay_ms: There is already a command in stack\n");
		return -1;
	}
}

// ------------------------------      ------------------------------
// ------------------------------ i2c read ------------------------------
//------------------------------       ------------------------------

int hermes_add_I2C_read(uint8_t addr, uint8_t len)
{

	uint8_t hermes_add_I2C_read_packet[] = {
		4,								// Total packet length
		Command_ID_USB_Device_I2C_Read, // Command ID
		addr,							// addr
		len,							// len
	};

	if (HERMES_SEND_VERBOSITY > 1)
	{
		printf("-LOG- VERBOSE SEND, hermes_add_I2C_read: Adding I2C read addr 0x%02X, len %i to stack\n", addr, len);
	}

	return hermes_packet_add_comand(hermes_add_I2C_read_packet, sizeof(hermes_add_I2C_read_packet));
}

int hermes_send_I2C_read(uint8_t addr, uint8_t len)
{
	bool error_code = 0;

	if (hermes_packet_check_heigh_taken() == 0)
	{
		error_code = hermes_add_I2C_read(addr, len);

		if (error_code < 0)
		{
			printf("-LOG- ERROR SEND, hermes_send_I2C_read: Error in adding command\n");
		}

		return hermes_packet_flush();
	}
	else
	{

		printf("-LOG- ERROR SEND, hermes_send_I2C_read: There is already a command in stack\n");
		return -1;
	}
}

// ------------------------------      ------------------------------
// ------------------------------ i2c send recieve ------------------------------
//------------------------------       ------------------------------

int hermes_add_I2C_send_recieve(uint8_t I2C_address, uint8_t write_len, uint8_t read_len, uint8_t *write_data)
{
	uint8_t command_len = 5 + write_len;
	uint8_t Send_Recieve_packet[] = {command_len, Command_ID_USB_Device_I2C_Send_Receive, I2C_address, write_len, read_len};

	if (HERMES_SEND_VERBOSITY > 1)
	{
		printf("-LOG- VERBOSE SEND, hermes_add_I2C_send_recieve: adding I2C send recieve. addr = %02X, write len = %i, read len = %i, write data is:\n-LOG-", I2C_address, write_len, read_len);
		for (int i = 0; i < write_len + 2; i++)
		{
			printf(" %02X", write_data[i]);
		}
	}
	hermes_packet_add_comand_without_advancing_stack_height(Send_Recieve_packet, 5);
	return hermes_packet_add_comand(write_data, write_len);
}

int hermes_send_I2C_send_recieve(uint8_t I2C_address, uint8_t write_len, uint8_t read_len, uint8_t *write_data)
{
	bool error_code = 0;

	if (hermes_packet_check_heigh_taken() == 0)
	{
		error_code = hermes_add_I2C_send_recieve(I2C_address, write_len, read_len, write_data);

		if (error_code < 0)
		{
			printf("-LOG- ERROR SEND, hermes_send_I2C_send_recieve: Error in adding command\n");
		}

		return hermes_packet_flush();
	}
	else
	{

		printf("-LOG- ERROR SEND, hermes_send_I2C_send_recieve: There is already a command in stack\n");
		return -1;
	}
}