#pragma once

#define E0 0
#define E1 1
#define E2 2
#define E3 3

/**
 * @brief
 * Write state of single led
 * @param E_LED Port of GPIO Expander, From E0 To E3
 */
void tca9554_led_write(unsigned int E_LED, unsigned int state);

/**
 * @brief
 * write low single led
 * @param E_LED Port of GPIO Expander, From E0 To E3
 */
void tca9554_led_reset(unsigned int E_LED);

/**
 * @brief
 * write high single led
 * @param E_LED Port of GPIO Expander, From E0 To E3
 */
void tca9554_led_set(unsigned int E_LED);

void tca9554_init(void);