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
#include <stdlib.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
/* --- MAIN FUNCTION (FIXED) --- */

int main(void)
{
	// 1. Setup Clocks
	rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);

	// Enable GPIO C (for LED) and GPIO A (for USB)
	rcc_periph_clock_enable(RCC_GPIOC);
	USB_initialization();

	// Visual indicator: Turn on LED (PC13 is usually active Low)
	gpio_clear(GPIOC, GPIO13);

	while (1)
		gpio_toggle(GPIOC, GPIO13);
}