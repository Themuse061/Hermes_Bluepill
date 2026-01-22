#include "ch32fun.h"
#include "i2c_slave.h"
#include <stdbool.h> // Include for bool type

// ===================================================================================
// CONFIGURATION & DEFINES
// ===================================================================================
#define I2C_ADDR 0x48

// Protocol Commands
#define I2C_Slave_Command_Reset_MCU 0x00
#define I2C_Slave_Command_Jump_To_Bootloader 0x01
#define I2C_Slave_Command_Flash_Set_Pointer 0x02
#define I2C_Slave_Command_Flash_Read_Page 0x03
#define I2C_Slave_Command_Flash_Write_Page 0x04
#define I2C_Slave_Command_Flash_Check_For_Error 0x05

// Memory Map
#define APP_START_ADDR 0x00000800
#define PAGE_SIZE 64
#define BUFFER_SIZE 255
#define BOOT_FLAG_ADDR ((volatile uint32_t *)0x200007FC)
#define BOOT_MAGIC_VALUE 0xBEEFCAFE

// Globals
volatile uint8_t i2c_buffer[BUFFER_SIZE];
volatile uint32_t flash_pointer = APP_START_ADDR;
volatile uint8_t need_to_write = 0;
volatile uint8_t last_error = 0;

// ===================================================================================
// LOW LEVEL HELPERS (Flash & Jump)
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

void JumpToApp(void)
{
    __disable_irq();
    RCC->APB1PCENR &= ~(RCC_APB1Periph_I2C1);
    asm volatile("csrw mtvec, %0" : : "r"(APP_START_ADDR | 1));
    void (*app_entry)(void) = (void (*)(void))APP_START_ADDR;
    app_entry();
    while (1)
        ;
}

// ===================================================================================
// I2C CALLBACKS
// ===================================================================================
void onWrite(uint8_t reg, uint8_t length)
{
    switch (reg)
    {
    case I2C_Slave_Command_Reset_MCU:
        *BOOT_FLAG_ADDR = 0;
        NVIC_SystemReset();
        while (1)
            ;
        break;
    case I2C_Slave_Command_Jump_To_Bootloader:
        *BOOT_FLAG_ADDR = BOOT_MAGIC_VALUE; /* Wait for Main Loop Logic */
        break;
    case I2C_Slave_Command_Flash_Set_Pointer:
        flash_pointer = 0x08000000 + (i2c_buffer[reg] | (i2c_buffer[reg + 1] << 8));
        break;
    case I2C_Slave_Command_Flash_Read_Page:
    {
        uint8_t *src = (uint8_t *)flash_pointer;
        for (int i = 0; i < PAGE_SIZE; i++)
            i2c_buffer[reg + i] = src[i];
    }
    break;
    case I2C_Slave_Command_Flash_Write_Page:
        need_to_write = 1;
        break;
    case I2C_Slave_Command_Flash_Check_For_Error:
        last_error = 0;
        break;
    }
}

void onRead(uint8_t reg)
{
    if (reg == I2C_Slave_Command_Flash_Check_For_Error)
        i2c_buffer[reg] = last_error;
}

// ===================================================================================
// MODULAR FUNCTIONS
// ===================================================================================

bool check_for_bootloader_with_wait()
{
    bool stay = false;

    // A: Did the Main App send us here via Soft Reset?
    if (*BOOT_FLAG_ADDR == BOOT_MAGIC_VALUE)
    {
        stay = true;
    }

    // B: Clear flag now.
    // If we return 'true', we stay in loop. If 'false', we jump.
    // Clearing it ensures next hard reset goes to App.
    *BOOT_FLAG_ADDR = 0;

    // C: Wait Window (Only if not already staying)
    if (!stay)
    {
        Delay_Ms(500);

        // Did I2C command arrive during the delay?
        // (onWrite sets this flag even during Delay_Ms)
        if (*BOOT_FLAG_ADDR == BOOT_MAGIC_VALUE)
        {
            stay = true;
        }
    }

    return stay;
}

void TheBootloaderLoop()
{
    // Visual Indication: Ready for commands
    funPinMode(PA2, GPIO_CFGLR_OUT_10Mhz_PP);
    funDigitalWrite(PA2, 1);

    while (1)
    {
        // 1. Handle Flash Writing
        if (need_to_write)
        {
            need_to_write = 0;

            // Get Pointers
            uint8_t *data_ptr = (uint8_t *)&i2c_buffer[I2C_Slave_Command_Flash_Write_Page];
            uint8_t recv_sum = i2c_buffer[I2C_Slave_Command_Flash_Write_Page + PAGE_SIZE];

            // Calculate Checksum
            uint8_t calc_sum = 0;
            for (int i = 0; i < PAGE_SIZE; i++)
                calc_sum += data_ptr[i];

            if (calc_sum == recv_sum)
            {
                if (flash_pointer >= APP_START_ADDR)
                {
                    funDigitalWrite(PA2, 0); // Blink OFF during write
                    Flash_ErasePage(flash_pointer);
                    Flash_ProgramPage(flash_pointer, data_ptr);
                    flash_pointer += PAGE_SIZE;
                    last_error = 0;
                    funDigitalWrite(PA2, 1); // Blink ON after write
                }
                else
                {
                    last_error = 2;
                } // Protected
            }
            else
            {
                last_error = 1;
            } // Bad Checksum
        }

        // 2. Check if Master ordered a Reset via 0x00 command
        // (Handled implicitly by ISR reset, but good to know loop is active)
    }
}

// ===================================================================================
// MAIN
// ===================================================================================
int main()
{
    SystemInit();
    funGpioInitAll();

    // 1. Sanity Check Blink (PD6)
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

    // 3. The Decision
    if (check_for_bootloader_with_wait())
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