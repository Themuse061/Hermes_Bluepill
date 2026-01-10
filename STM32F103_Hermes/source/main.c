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
/* --- MAIN FUNCTION (FIXED) --- */
void hardware_initalization(void);

void write_ladder_red(int value)
{
	uint16_t state = gpio_port_read(GPIOA);
	state = state & (value & 0b11111111);
	state |= value;
	gpio_port_write(GPIOA, state);
}



int main(void)
{

	hardware_initalization();

	gpio_set(GPIOA, 0b11111111);
	gpio_set(GPIOB, GPIO0 | GPIO1 | GPIO10 | GPIO11);

	while (1)
	{
		for (int i = 0; i < 0b11111111; i++)
		{
			write_ladder_red(i);
			delay_ms(50);
		}
	}
}

void __attribute__((weak)) USB_recieve_interrupt()
{
	char buf[64];
	int len = USB_read_data(buf, 64);

	if (len)
	{
		USB_send_data(buf, len);
	}

	gpio_toggle(GPIOC, GPIO13);
}