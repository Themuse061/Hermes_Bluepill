
#include "usb_handling.h"
#include "systick.h"
#include <stdlib.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/i2c.h>
#include <TCA9554.h>
#include <string.h>

#define USB_Command_Byte_Length 0
#define USB_Command_Byte_Command 1
#define USB_Command_Byte_Data 2

void USB_command_handler_I2C_write(char *command_array)
{
	char address = command_array[USB_Command_Byte_Data];
	char *data_start = &command_array[USB_Command_Byte_Data + 1];
	char data_length = command_array[USB_Command_Byte_Length] - 2;

	i2c_transfer7(I2C1, address, data_start, data_length, 0, 0);
}

/*
TCA write
0x38 0x01 0x00 ->
add reg value

lenght Command data
04013801FF

*/