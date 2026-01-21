#include "i2c_slave.h"
#include "ch32fun.h"
#include <stdio.h>

// The I2C slave library uses a one byte address so you can extend the size of this array up to 256 registers
// note that the register set is modified by interrupts, to prevent the compiler from accidently optimizing stuff
// away make sure to declare the register array volatile

volatile uint8_t i2c_registers[32] = {0x00};

void onWrite(uint8_t reg, uint8_t length)
{
	funDigitalWrite(PA2, i2c_registers[0] & 1);
	funDigitalWrite(PD6, i2c_registers[0] & 0b10);
	funDigitalWrite(PC4, i2c_registers[0] & 0b100);
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
