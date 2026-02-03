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
#define PAGE_SIZE 64
#define BUFFER_SIZE 255
#define BOOT_FLAG_ADDR ((volatile uint32_t *)0x200007FC)
#define BOOT_MAGIC_VALUE 0xBEEFCAFE

// Globals
volatile uint8_t i2c_buffer[BUFFER_SIZE];
volatile uint32_t flash_pointer = APP_START_ADDR;
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
    case Command_ID_I2C_Slave_Flash_Set_Pointer:
        flash_pointer = 0x08000000 + (i2c_buffer[reg] | (i2c_buffer[reg + 1] << 8));
        break;
    case Command_ID_I2C_Slave_Flash_Read_Page:
    {
        // Read Simulation: Copy from Flash (safe to read)
        uint8_t *src = (uint8_t *)flash_pointer;
        for (int i = 0; i < PAGE_SIZE; i++)
            i2c_buffer[reg + i] = src[i];
    }
    break;
    case Command_ID_I2C_Slave_Flash_Write_Page:
        need_to_write = 1;
        break;
    case Command_ID_I2C_Slave_Flash_Check_For_Error:
        break;
    }
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
    SetupI2CSlave(I2C_ADDR, i2c_buffer, sizeof(i2c_buffer), onWrite, NULL, false);

    // 6. Decision Logic
    uint8_t stay = 0;

    // CHECK 1: Missing Application (Panic Mode)
    // Note: If you have previously flashed the App, this will pass.
    // If chip is empty, it will trigger Panic Blinks.
    if (*(uint32_t *)APP_START_ADDR == 0xFFFFFFFF)
    {
        stay = 1;
        // DIAGNOSTIC: PANIC PATTERN (3 Slow Blinks)
        for (int i = 0; i < 3; i++)
        {
            GPIOD->BSHR = (1 << 6);
            simple_delay(2000000);
            GPIOD->BCR = (1 << 6);
            simple_delay(2000000);
        }
    }

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
        if (need_to_write)
        {
            need_to_write = 0;
            GPIOC->BSHR = (1 << 4); // PC4 ON (Activity LED)

            uint8_t *data_ptr = (uint8_t *)&i2c_buffer[Command_ID_I2C_Slave_Flash_Write_Page];
            uint8_t recv_sum = i2c_buffer[Command_ID_I2C_Slave_Flash_Write_Page + PAGE_SIZE];
            uint8_t calc_sum = 0;
            uint8_t err_code = 0;

            for (int i = 0; i < PAGE_SIZE; i++)
                calc_sum += data_ptr[i];

            // SIMULATED WRITE LOGIC
            if (calc_sum == recv_sum)
            {
                if (flash_pointer >= APP_START_ADDR)
                {
                    // --- FAKE WRITE ---
                    // We simply advance the pointer and pretend we wrote it.
                    // NO FLASH REGISTERS TOUCHED.

                    // Artificial delay to simulate flash write time (~5ms)
                    // 200,000 cycles approx 4ms
                    simple_delay(200000);

                    flash_pointer += PAGE_SIZE;
                    err_code = 0; // Success
                }
                else
                {
                    err_code = 2;
                } // Protected
            }
            else
            {
                err_code = 1;
            } // Checksum

            i2c_buffer[Command_ID_I2C_Slave_Flash_Check_For_Error] = err_code;
            GPIOC->BCR = (1 << 4); // PC4 OFF
        }
    }
}