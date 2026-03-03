#include "hermes_header.h"
#include "string.h"
#include "Command_ID.h"
#include "helper.h"

int hermes_easy_I2C_reset(uint8_t addr)
{
	if (HERMES_EASY_VERBOSITY > 1)
	{
		printf("-LOG- VERBOSE EASY, hermes_easy_I2C_reset: Reseting I2C addr 0x%02X, \n", addr);
	}

	uint8_t hermes_reset_packet[] = {Command_ID_I2C_Slave_Reset_MCU};
	int return_code = hermes_send_I2C_write(addr, hermes_reset_packet, 1);
	delay_ms(300);
	return return_code;
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
	int return_code = hermes_packet_flush();
	delay_ms(500);
	return return_code;
}

int hermes_easy_I2C_add_send_flash_pointer(uint8_t addr, uint32_t Flash_pointer)
{
	if (HERMES_EASY_VERBOSITY > 1)
	{
		printf("-LOG- VERBOSE EASY, hermes_easy_I2C_add_send_flash_pointer: addr 0x%02X, pointer %02X \n", addr, Flash_pointer);
	}

	uint8_t command_set_pointer[] = {Command_ID_I2C_Slave_Flash_Set_Pointer, 0, 0, 0, 0};
	command_set_pointer[1] = Flash_pointer & 0xFF; // low byte
	command_set_pointer[2] = (Flash_pointer >> 8) & 0xFF;
	command_set_pointer[3] = (Flash_pointer >> 16) & 0xFF;
	command_set_pointer[4] = (Flash_pointer >> 24) & 0xFF; // high byte

	return hermes_add_I2C_write(addr, command_set_pointer, 5);
}

// takes 3 command slots
int hermes_easy_I2C_add_read_flash(uint8_t addr, int amount)
{
	if (HERMES_EASY_VERBOSITY > 1)
	{
		printf("-LOG- VERBOSE EASY, hermes_easy_I2C_add_read_flash: addr 0x%02X, amount %i \n", addr, amount);
	}

	// ask for a write
	uint8_t Flash_read_ask_for_read[] = {Command_ID_I2C_Slave_Flash_Read_Page};
	hermes_add_I2C_write(addr, Flash_read_ask_for_read, 1);
	hermes_add_delay_ms(50);

	// read the write
	hermes_add_I2C_send_recieve(addr, 1, amount, Flash_read_ask_for_read);
	return 1;
}

int hermes_easy_i2C_add_write_flash_64_bytes(uint8_t addr, uint8_t *data)

{

	if (HERMES_EASY_VERBOSITY > 1)
	{
		printf("-LOG- VERBOSE EASY, hermes_easy_i2C_add_write_flash_64_bytes: addr 0x%02X, data ", addr);
		for (int i = 0; i < 64; i++)
		{
			printf("%02X ", data[i]);
		}
		printf("\n");
	}

	uint8_t flash_write_packet[65];
	flash_write_packet[0] = Command_ID_I2C_Slave_Flash_Write_Page;

	uint8_t *destination = &flash_write_packet[1];
	uint8_t *source = data;
	int length = 64;
	memcpy(destination, source, length);

	return hermes_add_I2C_write(addr, data, 65);
}
