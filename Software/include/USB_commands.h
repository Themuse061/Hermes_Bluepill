#pragma once

#include "stdint.h"

int Stack_add_ping(void);
int Stack_add_echo(uint8_t *data, uint8_t len);
int Stack_add_I2C_Write(uint8_t I2C_address, uint8_t *data, uint8_t len);
int Stack_add_delay(int delay);