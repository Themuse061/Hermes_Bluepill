#include "ch32v003fun.h"
#include <stdio.h>

int main()
{
	SystemInit();

	// Enable GPIOC
	RCC->APB2PCENR |= RCC_APB2Periph_GPIOC;

	// GPIO PC1 Push-Pull, 10MHz
	GPIOC->CFGLR &= ~(0xf << (4 * 1));
	GPIOC->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP) << (4 * 1);

	while (1)
	{
		GPIOC->BSHR = 1 << 1;     // Turn on PC1
		Delay_Ms(500);
		GPIOC->BSHR = 1 << (16 + 1); // Turn off PC1
		Delay_Ms(500);
	}
}
