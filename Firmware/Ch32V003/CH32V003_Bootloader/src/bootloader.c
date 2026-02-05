#include "ch32fun.h"
#include "i2c_slave.h"

// ===================================================================================
// CONFIGURATION
// ===================================================================================
#define I2C_ADDR 0x48

// Protocol Commands
#include <Command_ID.h>

// Memory
#define APP_START_ADDR 0x00000800
#define BUFFER_SIZE 255
#define BOOT_FLAG_ADDR ((volatile uint32_t *)0x200007FC)
#define BOOT_MAGIC_VALUE 0xBEEFCAFE

// Globals
volatile uint8_t i2c_buffer[BUFFER_SIZE];
volatile uint32_t flash_pointer = 0x08000000;
volatile uint8_t need_to_write = 0;

// ===================================================================================
// UTILS
// ===================================================================================
void simple_delay(int cycles)
{
    while (cycles--)
    {
        __asm__("nop");
    }
}

void raw_reset()
{
    PFIC->SCTLR = (1 << 31) | (1 << 2);
    while (1)
        ;
}

uint8_t safe_flash_read()
{
    // CH32V003 Flash is 16KB: 0x08000000 to 0x08004000
    if (flash_pointer >= 0x08000000 && flash_pointer < 0x08004000)
    {
        return *(uint8_t *)flash_pointer;
    }
    return 0xFF; // Return dummy data instead of crashing
}

// ===================================================================================
// I2C CALLBACK
// ===================================================================================
void onWrite(uint8_t reg, uint8_t length)
{
    switch (reg)
    {
    case Command_ID_I2C_Slave_Reset_MCU:
        *BOOT_FLAG_ADDR = 0;
        raw_reset();
        break;

    case Command_ID_I2C_Slave_Jump_To_Bootloader:
        *BOOT_FLAG_ADDR = BOOT_MAGIC_VALUE;
        break;

    // Command 0x02: Set Pointer
    case Command_ID_I2C_Slave_Flash_Set_Pointer:
        // Packet: [0x02] [Low] [High]
        // In ch32fun i2c_slave, buffer[reg] is the Low Byte.
        {
            uint16_t offset = i2c_buffer[reg] | (i2c_buffer[reg + 1] << 8);
            flash_pointer = 0x08000000 + offset;
        }
        break;

    // Command 0x03: Prepare for Read
    case Command_ID_I2C_Slave_Flash_Read_Page:
        // PRE-LOAD FIX: Load the buffer immediately using the safe function.
        // We do NOT increment the pointer here (onRead handles it).
        i2c_buffer[reg] = safe_flash_read();
        break;

    case Command_ID_I2C_Slave_Flash_Write_Page:
        need_to_write = 1;
        break;
    }
}

void onRead(uint8_t reg)
{
    // 1. Read byte safely
    uint8_t data = safe_flash_read();

    // 2. Put into buffer
    i2c_buffer[reg] = data;

    // 3. Increment pointer for the NEXT read
    flash_pointer++;
}

// ===================================================================================
// MAIN
// ===================================================================================
int main()
{
    SystemInit();

    // 1. Enable Clocks (A, C, D)
    RCC->APB2PCENR |= RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD;

    // 2. Configure LEDs
    GPIOD->CFGLR &= ~(0xF << 24);
    GPIOD->CFGLR |= (0x1 << 24); // PD6
    GPIOA->CFGLR &= ~(0xF << 8);
    GPIOA->CFGLR |= (0x1 << 8); // PA2
    GPIOC->CFGLR &= ~(0xF << 16);
    GPIOC->CFGLR |= (0x1 << 16); // PC4

    // 3. Configure I2C Pins (PC1, PC2) -> AF Open Drain
    GPIOC->CFGLR &= ~(0xFF << 4);
    GPIOC->CFGLR |= (0xDD << 4);

    // 4. DIAGNOSTIC: STARTUP PATTERN (5 Rapid Blinks)
    for (int i = 0; i < 5; i++)
    {
        GPIOD->BSHR = (1 << 6);
        simple_delay(100000);
        GPIOD->BCR = (1 << 6);
        simple_delay(200000);
    }

    // 5. Init I2C
    SetupI2CSlave(I2C_ADDR, i2c_buffer, sizeof(i2c_buffer), onWrite, onRead, false);

    // 6. Decision Logic
    uint8_t stay = 0;

    // CHECK 2: Soft Reset Flag
    if (*BOOT_FLAG_ADDR == BOOT_MAGIC_VALUE)
        stay = 1;
    *BOOT_FLAG_ADDR = 0;

    // CHECK 3: Window of Opportunity
    if (!stay)
    {
        simple_delay(2000000); // ~500ms Wait
        if (*BOOT_FLAG_ADDR == BOOT_MAGIC_VALUE)
            stay = 1;
    }

    // 7. Jump Logic
    if (!stay)
    {
        // Turn OFF LEDs
        GPIOD->BCR = (1 << 6);
        GPIOA->BCR = (1 << 2);
        GPIOC->BCR = (1 << 4);
        __disable_irq();
        RCC->APB1PCENR &= ~(RCC_APB1Periph_I2C1);
        asm volatile("csrw mtvec, %0" : : "r"(APP_START_ADDR | 1));
        void (*app_entry)(void) = (void (*)(void))APP_START_ADDR;
        app_entry();
        while (1)
            ;
    }

    // 8. Bootloader Loop (PA2 ON)
    GPIOA->BSHR = (1 << 2);

    while (1)
    {
    }
}