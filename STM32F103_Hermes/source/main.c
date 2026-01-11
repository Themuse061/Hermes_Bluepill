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
#include "systick.h"
#include <stdlib.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/i2c.h>
#include <TCA9554.h>

/* --- MAIN FUNCTION (FIXED) --- */
void hardware_initalization(void);

void write_ladder_red(int value)
{
	uint16_t state = gpio_port_read(GPIOA);
	state = state & (value & 0b11111111);
	state |= value;
	gpio_port_write(GPIOA, state);
}

char buf[64];
int len;
int sent = 0;

uint8_t full[] = {0xff, 0xcc, 0x17, 0b10101010};
uint8_t empty[] = {0x00, 0x00, 0x00, 0x00};

int main(void)
{

	hardware_initalization();

	write_ladder_red(0);
	gpio_clear(GPIOB, GPIO0 | GPIO1 | GPIO10 | GPIO11);

	while (1)
	{

		write_ladder_red(0);
		delay_ms(2000);
		write_ladder_red(1);
		i2c_transfer7(I2C1, 0x28, full, 4, 0, 0);
		write_ladder_red(2);
		delay_ms(2000);
		write_ladder_red(4);
		i2c_transfer7(I2C1, 0x28, empty, 4, 0, 0);
		write_ladder_red(8);
		delay_ms(2000);
	}
}

void USB_recieve_interrupt()
{

	len = USB_read_data(buf, 64);

	if (len)
	{
		USB_send_data(buf, len);
		sent = 1;
	}
}