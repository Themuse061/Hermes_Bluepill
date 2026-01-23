#include "tca9554.h"
#include "libopencm3/stm32/i2c.h"
#include "stdint.h"

static uint32_t tca9554_led_state = 0;

#define tca9554_ADDR 0x38   // A2,A1,A0 = GND
#define REG_INPUT     0x00
#define REG_OUTPUT    0x01
#define REG_POLARITY  0x02
#define REG_CONFIG    0x03


static void tca9554_write(uint8_t reg, uint8_t value) {
    uint8_t buf[2] = { reg, value };
    i2c_transfer7(I2C1, tca9554_ADDR, buf, 2, NULL, 0);
}


/**
 * @brief intiialize tca9554
 */
void tca9554_init(void) {
    // Set all pins as outputs (0 = output)
    tca9554_write(REG_CONFIG, 0x00);
    // Optionally clear outputs at start
    tca9554_write(REG_OUTPUT, 0x00);
}


void tca9554_led_set(unsigned int E_LED){

	tca9554_led_state |= 1 << E_LED;
	tca9554_write(REG_OUTPUT, tca9554_led_state);
}


void tca9554_led_reset(unsigned int E_LED){

	tca9554_led_state &= ~(1 << E_LED);
	tca9554_write(REG_OUTPUT, tca9554_led_state);
}


void tca9554_led_write(unsigned int E_LED, unsigned int state){

	if (state > 0)
	{
		tca9554_led_set(E_LED);
	} else {
		tca9554_led_reset(E_LED);
	}
}