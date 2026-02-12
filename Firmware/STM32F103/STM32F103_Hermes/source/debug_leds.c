#include <libopencm3/stm32/gpio.h>
#include "debug_leds.h"
#include "pinout.h"

void debug_led_usb_busy(bool state)
{
	if (state)
	{
		gpio_set(LED_green_1_port, LED_green_1_pin);
	}
	else
	{
		gpio_clear(LED_green_1_port, LED_green_1_pin);
	}
}

void debug_led_parsing_usb_command(bool state)
{
	if (state)
	{
		gpio_set(LED_green_2_port, LED_green_2_pin);
	}
	else
	{
		gpio_clear(LED_green_2_port, LED_green_2_pin);
	}
}

void debug_led_i2c_busy(bool state)
{
	// GPIO B0
	if (state)
	{
		gpio_set(LED_yellow_1_port, LED_yellow_1_pin);
	}
	else
	{
		gpio_clear(LED_yellow_1_port, LED_yellow_1_pin);
	}
}