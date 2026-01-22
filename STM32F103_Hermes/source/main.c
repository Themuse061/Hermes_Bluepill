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
	}
}

void USB_recieve_interrupt()
{
	write_ladder_red(2);
	int len = USB_read_data(buf, 256);
	write_ladder_red(4);

	if (len) // If any data was read
	{

		// Put commands into USB_Commands
		int current_length = 0;
		while ((current_length + 1) < len)
		{

			// Uses length to copy commands into their arrays
			memcpy(&USB_Commands[current_length][0], &buf[current_length], buf[current_length] + 1);

			// moves processed data pointer (current_length) to first byte of new data
			current_length += buf[current_length];
		}

		// Execute the commands
		for (int i = 0; USB_Commands[i][USB_Command_Byte_Length] != 0; i++)
		{
			switch (USB_Commands[i][USB_Command_Byte_Command])
			{
			case USB_Device_Command_I2C_Write:
				USB_command_handler_I2C_write(&USB_Commands[i][0]);
				break;

			case USB_Device_Command_I2C_Send_Receive:
				USB_command_handler_I2C_send_recieve(&USB_Commands[i][0]);
				break;

			case USB_Device_Command_Echo:
				USB_command_handler_echo(&USB_Commands[i][0]);
				break;

			case USB_Device_Command_Ping:
				USB_command_handler_ping(&USB_Commands[i][0]);
				break;

			case USB_Device_Command_Delay_Ms:
				USB_command_handler_delay_ms(&USB_Commands[i][0]);
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