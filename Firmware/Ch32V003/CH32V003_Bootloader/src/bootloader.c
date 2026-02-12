

// ===================================================================================
// CH32v003 J4M6 PINOUT
// ===================================================================================
/*
Pin 1 PD6
Pin 2           GND
Pin 3 PA2
Pin 4           VCC
Pin 5
Pin 6           SCL
Pin 7 PC4
Pin 8           PD1 SWIO
*/

#include "ch32fun.h"
#include "ch32v003hw.h"
#include "i2c_slave.h"

// ===================================================================================
// CONFIGURATION
// ===================================================================================

// Protocol Commands
#include <Command_ID.h>

// Memory
#define BOOT_FLAG_ADDR ((volatile uint32_t *)0x200007FC)
#define BOOT_MAGIC_VALUE 0xBEEFCAFE

// Flash Defines
// CH32V003 Flash is 16KB: 0x08000000 to 0x08004000
#define FLASH_START 0x08000000
#define FLASH_END 0x08004000
#define APP_START_ADDR 0x00001800 // 6 KB bootloader
#define FLASH_PAGE_SIZE 64
#define FLASH_PAGE_AMOUNT 256

// I2C Defines
#define I2C_BUFFER_SIZE 255
#define I2C_ADDR 0x48

// Globals
volatile uint8_t i2c_buffer[I2C_BUFFER_SIZE];
volatile uint32_t flash_pointer = 0x08000000;
volatile uint8_t need_to_write = 0;
uint8_t bootloader_version[] = {0x01, 0x00, 0x01, 0x01};

volatile bool master_sent_Flash_Read_Page;
volatile bool master_sent_Flash_Write_Page;
volatile bool master_sent_Flash_Get_Version;

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

    if (flash_pointer >= FLASH_START && flash_pointer < FLASH_END)
    {
        return *(uint8_t *)flash_pointer;
    }
    return 0xCA; // Return dummy data instead of crashing
}

void Enable_I2C(bool state)
{
    if (state)
    {
        I2C1->CTLR1 |= I2C_CTLR1_ACK;
        funDigitalWrite(PA2, FUN_LOW);
    }
    else
    {
        I2C1->CTLR1 &= ~I2C_CTLR1_ACK;
        funDigitalWrite(PA2, FUN_HIGH);
    }
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
        if (length >= 2) // Ensure we actually got 2 bytes
        {
            // lower byte in [0], higher in [4]
            flash_pointer = 0;
            flash_pointer |= (i2c_buffer[Command_ID_I2C_Slave_Flash_Set_Pointer + 0]) << 0; // low byte first
            flash_pointer |= (i2c_buffer[Command_ID_I2C_Slave_Flash_Set_Pointer + 1]) << 8;
            flash_pointer |= (i2c_buffer[Command_ID_I2C_Slave_Flash_Set_Pointer + 2]) << 16;
            flash_pointer |= (i2c_buffer[Command_ID_I2C_Slave_Flash_Set_Pointer + 3]) << 24;
        }
        break;

    case Command_ID_I2C_Slave_Flash_Get_Version:
        Enable_I2C(0);
        master_sent_Flash_Get_Version = 1;
        break;

    case Command_ID_I2C_Slave_Flash_Read_Page:
        Enable_I2C(0);
        master_sent_Flash_Read_Page = 1;
        break;

    default:
        break;
    }
}

// ===================================================================================
// Check for jump
// ===================================================================================

void check_for_jump_to_main()
{

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
        funDigitalWrite(PD6, FUN_LOW);
        funDigitalWrite(PA2, FUN_LOW);
        funDigitalWrite(PC4, FUN_LOW);
        __disable_irq();
        RCC->APB1PCENR &= ~(RCC_APB1Periph_I2C1);
        asm volatile("csrw mtvec, %0" : : "r"(APP_START_ADDR | 1));
        void (*app_entry)(void) = (void (*)(void))APP_START_ADDR;
        app_entry();
        while (1)
            ;
    }
}

// ===================================================================================
// MAIN
// ===================================================================================
int main()
{

    SystemInit();
    funGpioInitAll();

    // 1. Enable Clocks (A, C, D)
    RCC->APB2PCENR |= RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD;
    funPinMode(PD6, GPIO_CFGLR_OUT_10Mhz_PP);
    funPinMode(PA2, GPIO_CFGLR_OUT_10Mhz_PP);
    funPinMode(PC4, GPIO_CFGLR_OUT_10Mhz_PP);

    // 3. Configure I2C Pins (PC1, PC2) -> AF Open Drain
    GPIOC->CFGLR &= ~(0xFF << 4);
    GPIOC->CFGLR |= (0xDD << 4);

    // 4. DIAGNOSTIC: STARTUP PATTERN (5 Rapid Blinks)
    for (int i = 0; i < 5; i++)
    {
        funDigitalWrite(PD6, FUN_HIGH);
        simple_delay(100000);
        funDigitalWrite(PD6, FUN_LOW);
        simple_delay(200000);
    }

    // 5. Init I2C
    SetupI2CSlave(I2C_ADDR, i2c_buffer, sizeof(i2c_buffer), onWrite, NULL, false);

    // 6. Decision Logic
    check_for_jump_to_main();

    // load dummy data to i2c
    for (int i = 0x4E; i < I2C_BUFFER_SIZE; i++)
    {
        i2c_buffer[i] = i - 0x4E;
    }

    funDigitalWrite(PD6, FUN_HIGH);

    while (1)
    {

        if (master_sent_Flash_Read_Page)
        {
            // Put flash pointer into buffer for debbuging
            i2c_buffer[Command_ID_I2C_Slave_Flash_Read_Page + 0] = ((flash_pointer) & 0xff); // low byte
            i2c_buffer[Command_ID_I2C_Slave_Flash_Read_Page + 1] = (flash_pointer >> 8) & 0xFF;
            i2c_buffer[Command_ID_I2C_Slave_Flash_Read_Page + 2] = (flash_pointer >> 16) & 0xFF;
            i2c_buffer[Command_ID_I2C_Slave_Flash_Read_Page + 3] = (flash_pointer >> 24) & 0xFF; // high byte

            master_sent_Flash_Read_Page = 0;
            Enable_I2C(1);
        }

        if (master_sent_Flash_Write_Page)

        { // Flash the flash
        }

        if (master_sent_Flash_Get_Version)
        {
            for (int i = 0; i < 4; i++)
            {
                i2c_buffer[0x16 + i] = bootloader_version[i];
            }
            master_sent_Flash_Get_Version = 0;
            Enable_I2C(1);
        }
    }
}