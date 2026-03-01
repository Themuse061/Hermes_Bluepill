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
#include <libopencm3/stm32/usart.h>
#include "debug_leds.h"
#include <stdarg.h>

// variables

uint8_t USB_Commands[USB_Command_max_command_amount][USB_Command_max_length] = {0};
volatile int USB_data_recieved = 0;

volatile uint8_t *usb_buffer_external;
volatile int usb_buffer_len_external;

uint32_t led_millis;
int led_state = 0;
// functions

// Your existing single char function
void usart_send_char(char c)
{
	// Replace 'USART1' with your specific handle if different
	usart_send_blocking(USART1, c);
}
// Helper: Send a raw string
void usart_send_string(const char *str)
{
	while (*str)
	{
		usart_send_char(*str++);
	}
}

// ---------------------------------------------------------
// 2. Custom Lightweight Printf (No <stdio.h> overhead)
// ---------------------------------------------------------
// Supports: %c (char), %s (string), %d (int), %x (hex)
void uart_printf(const char *format, ...)
{
	va_list args;
	va_start(args, format);

	char buffer[16]; // Buffer for number conversions

	while (*format)
	{
		if (*format != '%')
		{
			usart_send_char(*format++);
			continue;
		}

		format++; // Skip '%'

		// --- Parse Flags & Width (Simple) ---
		int width = 0;
		int zero_pad = 0;

		// Check for zero padding '0'
		if (*format == '0')
		{
			zero_pad = 1;
			format++;
		}

		// Parse width (e.g., '2' in %02X)
		while (*format >= '0' && *format <= '9')
		{
			width = width * 10 + (*format - '0');
			format++;
		}

		// --- Parse Specifiers ---
		switch (*format)
		{
		case 'c': // Char
			usart_send_char((char)va_arg(args, int));
			break;

		case 's': // String
		{
			char *s = va_arg(args, char *);
			while (*s)
				usart_send_char(*s++);
		}
		break;

		case 'd': // Integer
		case 'i': // Integer (synonym)
		{
			int val = va_arg(args, int);
			int i = 0;

			if (val < 0)
			{
				usart_send_char('-');
				val = -val;
			}

			if (val == 0)
				buffer[i++] = '0';

			while (val > 0)
			{
				buffer[i++] = (val % 10) + '0';
				val /= 10;
			}

			// Handle Padding
			while (i < width)
			{
				usart_send_char(zero_pad ? '0' : ' ');
				width--;
			}

			// Print Reverse
			while (i > 0)
				usart_send_char(buffer[--i]);
		}
		break;

		case 'x': // Hex (lower)
		case 'X': // Hex (upper)
		{
			unsigned int val = va_arg(args, unsigned int);
			int i = 0;
			const char *hex_chars = (*format == 'x') ? "0123456789abcdef" : "0123456789ABCDEF";

			if (val == 0)
				buffer[i++] = '0';

			while (val > 0)
			{
				buffer[i++] = hex_chars[val % 16];
				val /= 16;
			}

			// Handle Padding (e.g. %02X needs 2 chars, if we have 1, print 1 '0')
			while (i < width)
			{
				usart_send_char(zero_pad ? '0' : ' ');
				width--;
			}

			while (i > 0)
				usart_send_char(buffer[--i]);
		}
		break;

		default: // Unrecognized, just print it literally (e.g. %%)
			usart_send_char('%');
			usart_send_char(*format);
			break;
		}
		format++;
	}

	va_end(args);
}

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

void USB_recieve_interrupt(uint8_t *recieve_buffer, int len)
{
	debug_led_usb_busy(1);

	// parse the commands to their arrays

	if (len) // If any data was read
	{
		// reset commands
		memset(USB_Commands, 0, sizeof(USB_Commands));

		// Put commands into USB_Commands
		int current_length = 0;
		int command_idx = 0;
		while ((current_length + 1) < len && command_idx < 16)
		{
			uint8_t cmd_len = recieve_buffer[current_length];

			// Uses length to copy commands into their arrays
			memcpy(&USB_Commands[command_idx][0], &recieve_buffer[current_length], cmd_len);

			// moves processed data pointer (current_length) to first byte of new data
			current_length += cmd_len;
			command_idx++;
		}

		// reset usb buffer
		for (int i = 0; i < len; i++)
		{
			// recieve_buffer[i] = 0;
		}
		usb_buffer_external = recieve_buffer;
		usb_buffer_len_external = len;
	}

	USB_data_recieved++; // execute the commands in main
	debug_led_usb_busy(0);
}

// main

int main(void)
{

	hardware_initalization();
	delay_ms(1000);
	tca9554_init();

	tca9554_led_write(E0, 1);
	tca9554_led_write(E1, 2);
	tca9554_led_write(E2, 1);

	write_ladder_red(0);
	debug_led_usb_busy(0);
	debug_led_i2c_busy(0);
	debug_led_parsing_usb_command(0);
	led_millis = get_tick();

	// main loop
	while (1)
	{

		// Led signaling
		if (get_tick() - led_millis >= 100)
		{
			if (led_state)
			{
				led_millis = get_tick();
				led_state = 0;
				write_ladder_red(0);
			}
			else
			{
				led_millis = get_tick();
				led_state = 1;
				write_ladder_red(1);
			}
		}

		// on USB recieve
		if (USB_data_recieved == 1)
		{
			uart_printf("\n\n\n----- Recieved new USB commands! -----\n");

			uart_printf("----- Raw USB Data -----\n");
			for (int j = 0; j < usb_buffer_len_external; j++)
			{
				uart_printf("%02X ", usb_buffer_external[j]);
			}
			uart_printf("\n");

			uart_printf("----- The commands: -----");
			for (int i = 0; i < 16 && USB_Commands[i][USB_Command_Byte_Length] != 0; i++)
			{
				uart_printf("\ncommand nr. %i: ", i);

				for (int j = 0; j < USB_Commands[i][USB_Command_Byte_Length]; j++)
				{
					uart_printf("%02X ", USB_Commands[i][j]);
				}
			}

			// recieve_buffer[i] = 0;
			debug_led_parsing_usb_command(1);

			// Execute the commands
			uart_printf("\n---------- executing commands... ----------");
			for (int i = 0; i < 16 && USB_Commands[i][USB_Command_Byte_Length] != 0; i++)
			{
				uart_printf("\nExecuting len: %02X, command %d\n", USB_Commands[i][USB_Command_Byte_Length], USB_Commands[i][USB_Command_Byte_Command]);
				for (int j = 0; j < USB_Commands[i][USB_Command_Byte_Length]; j++)
				{
					uart_printf("%02X ", USB_Commands[i][j]);
				}
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

			// reset led and flags
			USB_data_recieved = 0;
			debug_led_parsing_usb_command(0);
		}
		else if (USB_data_recieved > 1)
		{
			write_ladder_red(15);
			while (1)
			{
				/* code */
			}
		}
	}
}