#include <systick.h>
#include <libopencm3/cm3/systick.h>
volatile uint32_t system_millis = 0;

void sys_tick_handler(void)
{
	system_millis++; // increment millisecond counter
}

uint32_t get_tick(void)
{
	return system_millis;
}

void delay_ms(uint32_t delay_ms)
{

	uint32_t start = system_millis;
	while ((system_millis - start) < delay_ms)
	{
		__asm__ volatile("nop");
	}
}
