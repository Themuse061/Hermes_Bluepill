#include "ch32fun.h"
#include "i2c_slave.h"

// ===================================================================================
// CONFIGURATION
// ===================================================================================

#define I2C_ADDR 0x48
#define I2C_BUFFER_SIZE 255
volatile uint8_t i2c_buffer[I2C_BUFFER_SIZE];

#define I2C_Slave_Command_Reset_MCU 0x00             // (universal)
#define I2C_Slave_Command_Jump_To_Bootloader 0x01    // (universal)
#define I2C_Slave_Command_Flash_Set_Pointer 0x02     // (universal)
#define I2C_Slave_Command_Flash_Read_Page 0x03       // (universal)
#define I2C_Slave_Command_Flash_Write_Page 0x04      // (universal)
#define I2C_Slave_Command_Flash_Check_For_Error 0x05 // (universal)

#define APP_START_ADDR 0x00000800
#define BOOT_FLAG_ADDR ((volatile uint32_t *)0x200007FC)
#define BOOT_MAGIC_VALUE 0xBEEFCAFE

#define PAGE_SIZE 64
uint8_t last_error = 0;
volatile uint32_t flash_pointer = APP_START_ADDR;
volatile uint8_t need_to_write = 0;

// ===================================================================================
// FLASH PRIMITIVES
// ===================================================================================
void Flash_Unlock()
{
    if ((FLASH->CTLR & FLASH_CTLR_LOCK) != 0)
    {
        FLASH->KEYR = 0x45670123;
        FLASH->KEYR = 0xCDEF89AB;
    }
}
void Flash_Lock() { FLASH->CTLR |= FLASH_CTLR_LOCK; }

void Flash_ErasePage(uint32_t addr)
{
    Flash_Unlock();
    FLASH->CTLR |= FLASH_CTLR_PER;
    FLASH->ADDR = addr;
    FLASH->CTLR |= FLASH_CTLR_STRT;
    while (FLASH->STATR & FLASH_STATR_BSY)
        ;
    FLASH->CTLR &= ~FLASH_CTLR_PER;
    Flash_Lock();
}

void Flash_ProgramPage(uint32_t addr, uint8_t *data)
{
    Flash_Unlock();
    FLASH->CTLR |= FLASH_CTLR_PG;
    for (int i = 0; i < PAGE_SIZE; i += 2)
    {
        uint16_t val = data[i] | (data[i + 1] << 8);
        *(volatile uint16_t *)(addr + i) = val;
        while (FLASH->STATR & FLASH_STATR_BSY)
            ;
    }
    FLASH->CTLR &= ~FLASH_CTLR_PG;
    Flash_Lock();
}

// ===================================================================================
// HELPER: Jump to Program B
// ===================================================================================
void JumpToApp(void)
{
    __disable_irq();
    RCC->APB1PCENR &= ~(RCC_APB1Periph_I2C1); // De-init I2C
    asm volatile("csrw mtvec, %0" : : "r"(APP_START_ADDR | 1));

    void (*app_entry)(void) = (void (*)(void))APP_START_ADDR;
    app_entry();

    while (1)
        ;
}

// ===================================================================================
// I2C CALLBACK
// ===================================================================================
void onWrite(uint8_t reg, uint8_t length)
{

    // 'reg' is the Command ID (the first byte sent)
    // The payload data has already been written to i2c_registers[reg], [reg+1], etc.

    switch (reg)
    {
    case I2C_Slave_Command_Reset_MCU:
        *BOOT_FLAG_ADDR = 0; // Clear flag
        NVIC_SystemReset();
        while (1)
            ;
        break;

    case I2C_Slave_Command_Jump_To_Bootloader:
        *BOOT_FLAG_ADDR = BOOT_MAGIC_VALUE; // Set flag
        break;

    case I2C_Slave_Command_Flash_Set_Pointer:
        // Packet: [0x02] [AddrLo] [AddrHi]
        // Data is at i2c_registers[2] and [3]
        {
            uint16_t offset = i2c_registers[reg] | (i2c_registers[reg + 1] << 8);
            flash_pointer = 0x08000000 + offset;
        }
        break;

    case I2C_Slave_Command_Flash_Read_Page:
        // Packet: [0x03] (Master writes this to set the index)
        // PREPARE BUFFER: Load 64 bytes from Flash into RAM immediately
        // so it is ready when the Master sends the RESTART+READ signal.
        {
            uint8_t *flash_src = (uint8_t *)flash_pointer;
            for (int i = 0; i < PAGE_SIZE; i++)
            {
                // We load data into the buffer starting at index 0x03
                i2c_registers[reg + i] = flash_src[i];
            }
        }
        break;

    case I2C_Slave_Command_Flash_Write_Page:
        // Packet: [0x04] [64 Bytes Data] [Checksum]
        // We cannot write Flash in an interrupt. Set flag for Main Loop.
        need_to_write = 1;
        break;

    case I2C_Slave_Command_Flash_Check_For_Error:
        // Usually read-only, but if Master writes here, we can clear errors.
        last_error = 0;
        break;

    default:
        break;
    }
}

void onRead(uint8_t reg)
{
    switch (reg)
    {
    case I2C_Slave_Command_Flash_Check_For_Error:
        // Just-in-time update: Ensure register holds the latest error state
        i2c_registers[reg] = last_error;
        break;

    case I2C_Slave_Command_Flash_Read_Page:
        // Optional: You could load data here byte-by-byte,
        // but pre-loading in onWrite is usually safer for timing.
        break;

    default:
        break;
    }
}
// ===================================================================================
// Bootloader Checking loop
// ===================================================================================

int check_for_bootloader_with_wait()
{
    bool stay_in_bootloader = false;

    // A: Did the Main App send us here via Soft Reset?
    if (*BOOT_FLAG_ADDR == BOOT_MAGIC_VALUE)
    {
        stay_in_bootloader = true;
    }

    // B: Clear flag now, so if we reset later we don't get stuck
    *BOOT_FLAG_ADDR = 0;

    // C: Give external Master 500ms to send command 0x01
    // (Only wait if we aren't already staying)
    if (!stay_in_bootloader)
    {
        Delay_Ms(500);

        // Did I2C command arrive during the delay?
        if (*BOOT_FLAG_ADDR == BOOT_MAGIC_VALUE)
        {
            stay_in_bootloader = true;
        }
    }

    return stay_in_bootloader;
}

// ===================================================================================
// The Bootloader
// ===================================================================================

void TheBootloaderLoop()
{
    // --- BOOTLOADER LOOP ---

    // Initialize other LEDs for the show
    funPinMode(PA2, GPIO_CFGLR_OUT_10Mhz_PP);
    funPinMode(PC4, GPIO_CFGLR_OUT_10Mhz_PP);

    // Turn OFF the debug LED to show we finished init
    funDigitalWrite(PD6, 0);

    while (1)
    {
        // Blink Pattern: FAST blink = Bootloader Mode
        funDigitalWrite(PA2, 1);
        Delay_Ms(100);
        funDigitalWrite(PA2, 0);
    }
}

// ===================================================================================
// MAIN
// ===================================================================================
int main()
{
    SystemInit();
    funGpioInitAll();

    // 1. FIX: Initialize LED Pin BEFORE writing to it
    // Using PD6 as debug LED
    funPinMode(PD6, GPIO_CFGLR_OUT_10Mhz_PP);
    for (int i = 0; i < 5; i++)
    {
        funDigitalWrite(PD6, 1);
        Delay_Ms(50);
        funDigitalWrite(PD6, 0);
        Delay_Ms(50);
    }

    // 2. Initialize I2C
    funPinMode(PC1, GPIO_CFGLR_OUT_10Mhz_AF_OD); // SDA
    funPinMode(PC2, GPIO_CFGLR_OUT_10Mhz_AF_OD); // SCL
    SetupI2CSlave(I2C_ADDR, i2c_buffer, sizeof(i2c_buffer), onWrite, onRead, false);

    // 4. The Decision
    if (check_for_bootloader_with_wait)
    {
        TheBootloaderLoop();
    }
    else
    {
        // Turn OFF debug LED before jumping
        funDigitalWrite(PD6, 0);

        // --- JUMP TO MAIN APP ---
        JumpToApp();
    }
}
