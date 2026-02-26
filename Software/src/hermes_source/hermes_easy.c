#include "hermes_header.h"
#include "string.h"
#include "Command_ID.h"

int hermes_easy_I2C_reset(uint8_t addr)
{
	if (HERMES_EASY_VERBOSITY > 1)
	{
		printf("-LOG- VERBOSE EASY, hermes_easy_I2C_reset: Reseting I2C addr 0x%02X, \n", addr);
	}

	uint8_t hermes_reset_packet[] = {Command_ID_I2C_Slave_Reset_MCU};
	return hermes_send_I2C_write(addr, hermes_reset_packet, 1);
}

int hermes_easy_I2C_jump_to_bootloader(uint8_t addr)
{
	if (HERMES_EASY_VERBOSITY > 1)
	{
		printf("-LOG- VERBOSE EASY, hermes_easy_I2C_jump_to_bootloader: Jumping I2C addr 0x%02X, \n", addr);
	}

	uint8_t command_reset[] = {Command_ID_I2C_Slave_Reset_MCU};
	uint8_t command_jump_to_bootloader[] = {Command_ID_I2C_Slave_Jump_To_Bootloader};

	hermes_add_I2C_write(addr, command_reset, 1);			   // reset
	hermes_add_delay_ms(250);								   // wait 250ms
	hermes_add_I2C_write(addr, command_jump_to_bootloader, 1); // try to jump
	hermes_packet_flush();
}
