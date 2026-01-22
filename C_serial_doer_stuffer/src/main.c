#include <stdio.h>
#include <stdlib.h>
#include "usb_serial.h"
#include "USB_commands.h"

// Configuration
const char *COM_PORT = "COM14";
const int BAUD_RATE = 115200;

#define I2C_Slave_Command_Reset_MCU 0x00		   // (universal)
#define I2C_Slave_Command_Jump_To_Bootloader 0x01  // (universal)
#define I2C_Slave_Command_Write_Flash 0x02		   // (universal)
#define I2C_Slave_Command_Read_Flash 0x03		   // (universal)
#define I2C_Slave_Command_Go_To_Flash_Address 0x04 // (universal)

#define I2C_Slave_Command_GPIO_Set 0x10 // (dark_times)

// USB Device Commands
#define USB_Device_Command_I2C_Write 0x01
#define USB_Device_Command_I2C_Send_Receive 0x02
#define USB_Device_Command_Echo 0x03
#define CMD_PING 0x04
#define USB_Device_Command_Delay_Ms 0x05

#define USB_Device_Command_PC_Short_Data_Return 0xFF

#define TCA_ADDRESS 0x38

int main()
{
	// 1. Setup Port
	if (USB_init(COM_PORT, BAUD_RATE) < 0)
	{
		fprintf(stderr, "Error: Failed to open port %s\n", COM_PORT);
		return 1;
	}

	printf("--- Starting USB Command Test ---\n");

	// 2. Ping
	USB_command_ping();

	// 3. Echo
	uint8_t echo_data[] = {0xDE, 0xAD, 0xBE, 0xEF};
	USB_command_echo(echo_data, sizeof(echo_data));

	// 4. I2C Write 1: 0x01 0x00 to TCA_ADDRESS
	uint8_t i2c_data1[] = {0x01, 0x00};
	USB_command_i2c_write(TCA_ADDRESS, i2c_data1, sizeof(i2c_data1));

	// 5. Delay 1s
	printf("... Delaying 1s ...\n");
	// Using USB_wait_for_data with 1000ms timeout acts as a sleep if no data arrives
	USB_wait_for_data(1000);

	// TCA blink one command
	uint8_t i2c_data_delay[] = {
		5, USB_Device_Command_I2C_Write, TCA_ADDRESS, 0x01, 0x00 3, USB_Device_Command_Delay_Ms, 255,
		5, USB_Device_Command_I2C_Write, TCA_ADDRESS, 0x01, 0xFF 3, USB_Device_Command_Delay_Ms, 255,
		5, USB_Device_Command_I2C_Write, TCA_ADDRESS, 0x01, 0x00};
	USB_write(i2c_data_delay, sizeof(i2c_data_delay));

	// 6. I2C Write 2: 0x01 0xFF to 0x38
	uint8_t i2c_data2[] = {0x01, TCA_ADDRESS};
	USB_command_i2c_write(TCA_ADDRESS, i2c_data2, sizeof(i2c_data2));

	//  I2C Write TO ch32 2: 0x01 0xFF to 0x38
	uint8_t i2c_data3[] = {0x10, 0xFF};

	for (int i = 0; i < 40; i++)
	{

		for (int j = 0; j < 8; j++)
		{
			i2c_data3[1] = j;
			printf("sending new value\n");
			USB_command_i2c_write(0x09, i2c_data3, sizeof(i2c_data2));
			USB_wait_for_data(40 - i);
		}
	}

	USB_wait_for_data(3000);
	printf("reseting mcu...");
	USB_wait_for_data(3000);
	i2c_data3[0] = 0x00;
	USB_command_i2c_write(0x09, i2c_data3, sizeof(i2c_data2));

	printf("--- Test Complete ---\n");

	// 7. Cleanup
	USB_deinit();

	return 0;
}