#include <libopencm3/cm3/cortex.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/i2c.h>
#include <libopencm3/stm32/usart.h>
#include "pinout.h"

#include "usb_handling.h"

void initialize_RCC()
{
	rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);
	/* hse8, pll to 72
	.pll_mul = RCC_CFGR_PLLMUL_PLL_CLK_MUL9,
	.pll_source = RCC_CFGR_PLLSRC_HSE_CLK,
	.hpre = RCC_CFGR_HPRE_NODIV,
	.ppre1 = RCC_CFGR_PPRE_DIV2,
	.ppre2 = RCC_CFGR_PPRE_NODIV,
	.adcpre = RCC_CFGR_ADCPRE_DIV8,
	.flash_waitstates = 2,
	.prediv1 = RCC_CFGR2_PREDIV_NODIV,
	.ahb_frequency = 72e6,
	.apb1_frequency = 36e6,
	.apb2_frequency = 72e6,
	*/

	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_GPIOB);
	rcc_periph_clock_enable(RCC_GPIOC);

	systick_set_clocksource(STK_CSR_CLKSOURCE_AHB); // AHB = core clock
	systick_set_reload(rcc_ahb_frequency / 1000);
	systick_interrupt_enable();
	systick_counter_enable();
}

void initialize_GPIO()
{
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_2_MHZ,
				  GPIO_CNF_OUTPUT_PUSHPULL, 0b11111111); // Red leds
	gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_2_MHZ,
				  GPIO_CNF_OUTPUT_PUSHPULL, LED_green_1_pin | LED_green_2_pin | LED_yellow_1_pin | LED_yellow_2_pin);
}

void initialize_I2C()
{

	/* Enable clocks for I2C1 and AFIO. */
	rcc_periph_clock_enable(RCC_I2C1);
	rcc_periph_clock_enable(RCC_AFIO);
	rcc_periph_clock_enable(RCC_GPIOB);

	/* Set alternate functions for the SCL and SDA pins of I2C1. */
	gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ,
				  GPIO_CNF_OUTPUT_ALTFN_OPENDRAIN,
				  GPIO6 | GPIO7);

	/* Disable the I2C before changing any configuration. */
	i2c_peripheral_disable(I2C1);

	/* set speed */
	i2c_set_speed(I2C1, i2c_speed_sm_100k, 36); // 36 MHz APB1

	/* If everything is configured -> enable the peripheral. */
	i2c_peripheral_enable(I2C1);
}

void initialize_USART()
{
	// Enable clocks
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_USART1);

	// Setup GPIO pins for USART1 TX/RX
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
				  GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_USART1_TX);
	gpio_set_mode(GPIOA, GPIO_MODE_INPUT,
				  GPIO_CNF_INPUT_FLOAT, GPIO_USART1_RX);

	// Setup USART parameters
	usart_set_baudrate(USART1, 115220);
	usart_set_databits(USART1, 8);
	usart_set_stopbits(USART1, USART_STOPBITS_1);
	usart_set_mode(USART1, USART_MODE_TX);
	usart_set_parity(USART1, USART_PARITY_NONE);
	usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);

	// Enable USART
	usart_enable(USART1);
}

void hardware_initalization()
{
	initialize_RCC();
	hermes_USB_initialization();
	initialize_GPIO();
	initialize_I2C();
	initialize_USART();
}
