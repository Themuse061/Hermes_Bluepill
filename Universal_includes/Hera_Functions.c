#include "helper.h"
#include "Hera_Functions.h"
#include "hermes_packet_sending.h"
#include "USB_commands.h"
#include "Command_ID.h"
#include "verbosity_settings.h"
#include "stdio.h"

int hermes_easy_I2C_reset(uint8_t addr)
{
	if (HERA_VERBOSE_LEVEL > 0)
	{
		printf("Resetting 0x%02X...\n", addr);
	}

	uint8_t command_reset[] = {Command_ID_I2C_Slave_Reset_MCU};
	hermes_add_I2C_write(addr, command_reset, 1);
	delay_ms(200);
	Hermes_Flush_Stack();
	delay_ms(2000);
	return 1;
}

int hermes_easy_I2C_jump_to_bootloader(uint8_t addr)
{
	if (HERA_VERBOSE_LEVEL > 0)
	{
		printf("Jumping to boot 0x%02X...\n", addr);
	}

	uint8_t command_reset[] = {Command_ID_I2C_Slave_Reset_MCU};
	uint8_t command_jump_to_bootloader[] = {Command_ID_I2C_Slave_Jump_To_Bootloader};

	hermes_add_I2C_write(addr, command_reset, 1);			   // reset
	hermes_add_delay_ms(250);								   // wait 50ms
	hermes_add_I2C_write(addr, command_jump_to_bootloader, 1); // try to jump
	Hermes_Flush_Stack();
	delay_ms(2000);

	return 1;
}