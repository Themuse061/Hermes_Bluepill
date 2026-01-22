#include "ch32fun.h"
#include "i2c_slave.h"
#include <stdio.h>

// ===================================================================================
// CONFIGURATION
// ===================================================================================
// Use 0x48 to match the Bootloader, or 0x09 if you prefer.
// Just ensure your PC host script sends to the right address.
#define I2C_ADDR 0x48

// Commands
#define I2C_Slave_Command_Reset_MCU 0x00
#define I2C_Slave_Command_Jump_To_Bootloader 0x01

// Magic Flag Location (Must match Bootloader!)
#define BOOT_FLAG_ADDR ((volatile uint32_t *)0x200007FC)
#define BOOT_MAGIC_VALUE 0xBEEFCAFE

// Minimal buffer for I2C (we don't need a huge one for just commands)
volatile uint8_t i2c_registers[32] = {0};

// ===================================================================================
// I2C CALLBACK
// ===================================================================================
void onWrite(uint8_t reg, uint8_t length)
{
    switch (reg)
    {
    case I2C_Slave_Command_Reset_MCU:
        // 1. Clear flag (Normal Reset)
        *BOOT_FLAG_ADDR = 0;

        // 2. Reset
        NVIC_SystemReset();
        while (1)
            ; // Wait for death
        break;

    case I2C_Slave_Command_Jump_To_Bootloader:
        // 1. Set Flag (Stay in Bootloader after reset)
        *BOOT_FLAG_ADDR = BOOT_MAGIC_VALUE;

        // 2. Reset (Crucial! The bootloader only runs at startup)
        NVIC_SystemReset();
        while (1)
            ; // Wait for death
        break;

    default:
        break;
    }
}

// ===================================================================================
// MAIN LOOP
// ===================================================================================
int main()
{
    SystemInit();
    funGpioInitAll();
    __enable_irq();

    // Initialize I2C slave
    funPinMode(PC1, GPIO_CFGLR_OUT_10Mhz_AF_OD); // SDA
    funPinMode(PC2, GPIO_CFGLR_OUT_10Mhz_AF_OD); // SCL
    SetupI2CSlave(I2C_ADDR, i2c_registers, sizeof(i2c_registers), onWrite, NULL, false);

    // Initialize LEDs
    funPinMode(PA2, GPIO_CFGLR_OUT_10Mhz_PP);
    funPinMode(PD6, GPIO_CFGLR_OUT_10Mhz_PP);
    funPinMode(PC4, GPIO_CFGLR_OUT_10Mhz_PP);

    while (1)
    {
        // Blink logic
        funDigitalWrite(PA2, 1);
        funDigitalWrite(PD6, 1);
        funDigitalWrite(PC4, 1);
        Delay_Ms(100);

        funDigitalWrite(PA2, 0);
        funDigitalWrite(PD6, 0);
        funDigitalWrite(PC4, 0);
        Delay_Ms(100);
    }
}