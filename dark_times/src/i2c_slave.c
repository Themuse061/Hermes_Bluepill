#include "i2c_slave.h"
#include "ch32fun.h"
#include <stdio.h>

// The I2C slave library uses a one byte address so you can extend the size of this array up to 256 registers
// note that the register set is modified by interrupts, to prevent the compiler from accidently optimizing stuff
// away make sure to declare the register array volatile

volatile uint8_t i2c_registers[255] = {0x00};

/**
 * Command list
 *
 * 0x00 - Reset MCU (non implemented)
 * 0x01 - Jump to bootloader
 * 0x02 - Write Flash
 * 0x03 - Read Flash
 * 0x04 - Go to flash address
 *
 * 0x10 - GPIO Set (implemented)
 */

#define I2C_Slave_Command_Reset_MCU 0x00
#define I2C_Slave_Command_Jump_To_Bootloader 0x01
#define I2C_Slave_Command_Write_Flash 0x02
#define I2C_Slave_Command_Read_Flash 0x03
#define I2C_Slave_Command_Go_To_Flash_Address 0x04

#define I2C_Slave_Command_GPIO_Set 0x10

void onWrite(uint8_t reg, uint8_t length)
{
	switch (reg)
	{
	case I2C_Slave_Command_Reset_MCU:
		NVIC_SystemReset();
		while (1)
		{
		}
		break;

	case I2C_Slave_Command_GPIO_Set:
		// Master sent: [Addr] [CMD_GPIO] [Bitmask]
		// Library put [Bitmask] into i2c_registers[CMD_GPIO]
		// So we read from i2c_registers[reg]

		if (length >= 1)
		{ // Safety check
			uint8_t val = i2c_registers[reg];

			funDigitalWrite(PA2, val & 1);
			funDigitalWrite(PD6, val & 0b10);
			funDigitalWrite(PC4, val & 0b100);
		}
		break;

	default:
		break;
	}
}
int main()
{
	SystemInit();
	funGpioInitAll();

	// Initialize I2C slave
	funPinMode(PC1, GPIO_CFGLR_OUT_10Mhz_AF_OD); // SDA
	funPinMode(PC2, GPIO_CFGLR_OUT_10Mhz_AF_OD); // SCL
	SetupI2CSlave(0x09, i2c_registers, sizeof(i2c_registers), onWrite, NULL, false);

	// Initialize LED
	funPinMode(PA2, GPIO_CFGLR_OUT_10Mhz_PP); // LED
	funPinMode(PD6, GPIO_CFGLR_OUT_10Mhz_PP); // LED
	funPinMode(PC4, GPIO_CFGLR_OUT_10Mhz_PP); // LED

	while (1)
	{
	} // Do not let main exit, you can do other things here
}
