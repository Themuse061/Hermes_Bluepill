#pragma once
#include <stdint.h>

/**
 * @brief Delays execution for a specified number of milliseconds.
 *
 * @param ms The number of milliseconds to delay.
 */
void delay_ms(int ms);

void helper_start_timer();
void helper_end_timer();
