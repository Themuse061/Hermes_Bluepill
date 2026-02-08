/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2010 Gareth McMullin <gareth@blacksphere.co.nz>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "usb_handling.h"
#include "USB_packet_handling.h"
#include "systick.h"
#include <stdlib.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/i2c.h>
#include <TCA9554.h>
#include <string.h>

void *memcpy(void *dest, const void *src, size_t n)
{
	unsigned char *d = dest;
	const unsigned char *s = src;
	for (; n; n--)
		*d++ = *s++;
	return dest;
}

void hardware_initalization(void);

void write_ladder_red(int value)
{
	uint16_t state = gpio_port_read(GPIOA);
	state = state & (value & 0b11111111);
	state |= value;
	gpio_port_write(GPIOA, state);
}

uint8_t buf[256];
uint8_t USB_Commands[16][USB_Command_max_length] = {0};
int USB_data_recieved = 0;

int main(void)
{

	hardware_initalization();
	delay_ms(1000);
	tca9554_init();

	tca9554_led_write(E0, 1);
	tca9554_led_write(E1, 2);
	tca9554_led_write(E2, 1);

	write_ladder_red(0);
	gpio_clear(GPIOB, GPIO0 | GPIO1 | GPIO10 | GPIO11);
	while (1)
	{
		delay_ms(800);
		write_ladder_red(0);
		delay_ms(800);
		write_ladder_red(1);
		if (USB_data_recieved == 1)
		{

			// Execute the commands
			for (int i = 0; i < 16 && USB_Commands[i][USB_Command_Byte_Length] != 0; i++)
			{
				switch (USB_Commands[i][USB_Command_Byte_Command])
				{
				case Command_ID_USB_Device_I2C_Write:
					USB_command_handler_I2C_write(&USB_Commands[i][0]);
					break;

				case Command_ID_USB_Device_I2C_Send_Receive:
					USB_command_handler_I2C_send_recieve(&USB_Commands[i][0]);
					break;

				case Command_ID_USB_Device_Echo:
					USB_command_handler_echo(&USB_Commands[i][0]);
					break;

				case Command_ID_USB_Device_Ping:
					USB_command_handler_ping(&USB_Commands[i][0]);
					break;

				case Command_ID_USB_Device_Delay_Ms:
					USB_command_handler_delay_ms(&USB_Commands[i][0]);
					break;

				case Command_ID_USB_Device_I2C_Read:
					USB_Command_handler_I2C_Read(&USB_Commands[i][0]);
					break;

				default:
					while (1)
					{
						// not implemented
					}

					break;
				}

				// Reset the command
				USB_Commands[i][USB_Command_Byte_Length] = 0;
			}
			USB_data_recieved = 0;
		}
		else if (USB_data_recieved)
		{
			// USB data recieve > 1 so there was command overflow
		}
	}
}

void USB_recieve_interrupt()
{
	write_ladder_red(2);
	int len = USB_read_data(buf, 256);
	write_ladder_red(4);

	if (len) // If any data was read
	{
		// reset commands
		memset(USB_Commands, 0, sizeof(USB_Commands));

		// Put commands into USB_Commands
		int current_length = 0;
		int command_idx = 0;
		while ((current_length + 1) < len && command_idx < 16)
		{
			uint8_t cmd_len = buf[current_length];

			// Validate length to prevent infinite loops or buffer overreads
			if (cmd_len == 0 || (current_length + cmd_len) > len)
			{
				break;
			}

			// Uses length to copy commands into their arrays
			memcpy(&USB_Commands[command_idx][0], &buf[current_length], cmd_len);

			// moves processed data pointer (current_length) to first byte of new data
			current_length += cmd_len;
			command_idx++;
		}
		USB_data_recieved++; // execute the commands in main
	}
}

/*
USB Packet
all uint8_t

USB -> uC
1. Length
2. Command type
3. Data

uC -> USB

*/

/*
commands

0x01 -> I2C write
	First byte - addr Xaaaaaaa
	rest (up to end of length) - data

0x02 -> I2C read register


*/