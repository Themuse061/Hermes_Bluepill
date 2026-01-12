
#include "usb_handling.h"
#include "USB_packet_handling.h"
#include "systick.h"
#include <stdlib.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/i2c.h>
#include <TCA9554.h>
#include <string.h>

/**
 * COMMAND 0x01 USB_command_handler_I2C_write
 *
 * Writes data to I2C1
 * FORMAT
 * uint8_t[0] - Packet Length
 * uint8_t[1] - This Command
 * uint8_t[2] - target address
 * uint8_t[3+] - data
 *	 	length of data is calculated from Packet length - 2
 */
void USB_command_handler_I2C_write(uint8_t *command_array)
{
	uint8_t address = command_array[USB_Command_Byte_Data];
	uint8_t *data_start = &command_array[USB_Command_Byte_Data + 1];
	uint8_t data_length = command_array[USB_Command_Byte_Length - 2];

	i2c_transfer7(I2C1, address, data_start, data_length, 0, 0);
}
/*
TCA write
0x38 0x01 0x00 ->
add reg value

lenght Command data
04013801FF -> on
0401380100 -> off

*/

/**
 * COMMAND 0x02 USB_command_handler_I2C_send_recieve
 *
 * Writes some data to I2C, then recieves some data
 * FORMAT
 * uint8_t[0] - Packet Length
 * uint8_t[1] - This Command
 * uint8_t[2] - target address
 * uint8_t[3] - how much to write
 * uint8_t[4] - how much data to read (max = USB_Command_data_size)
 * uint8_t[5+] - data to write
 *
 * first writes, then sends
 */
void USB_command_handler_I2C_send_recieve(uint8_t *command_array)
{
	uint8_t address = command_array[2];
	uint8_t write_length = command_array[3];
	uint8_t read_length = command_array[4];
	uint8_t *write_data = &command_array[5];

	uint8_t read_data[USB_Command_data_size] = {0};

	i2c_transfer7(I2C1, address, write_data, write_length, read_data, read_length);

	// Send USB_command_PC_short_data_return to PC
	uint8_t USB_send_buffer[USB_Command_max_length] = {0};
	USB_send_buffer[0] = read_length + 4; // Packet Length
	USB_send_buffer[1] = USB_command_PC_short_data_return;
	USB_send_buffer[2] = USB_command_handler_I2C_send_recieve_number;
	USB_send_buffer[3] = address;
	memcpy(&USB_send_buffer[4], &read_data[0], read_length);

	USB_send_data(USB_send_buffer, (uint16_t)USB_send_buffer[0]);
}

/*
========================================== uC -> PC COMMANDS ==========================================
*/

/**
 * COMMAND 0xFF USB_command_PC_short_data_return
 *
 * Returns data when calling various operations
 * FORMAT
 * uint8_t[0] - Packet Length
 * uint8_t[1] - This Command
 * uint8_t[2] - What command called this command
 * 		For command 0x02 USB_command_handler_I2C_send_recieve
 *			uint8_t[3] - Address which replied
 *			uint8_t[4+] - returned data
 */
