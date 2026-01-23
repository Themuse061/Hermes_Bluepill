#include "ch32fun.h"
#include "i2c_slave.h"

// ===================================================================================
// CONFIGURATION
// ===================================================================================
#define I2C_ADDR 0x48

// Protocol Commands
#define CMD_RESET 0x00
#define CMD_JUMP_BOOT 0x01
#define CMD_SET_PTR 0x02
#define CMD_READ 0x03
#define CMD_WRITE 0x04
#define CMD_CHECK_ERR 0x05

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

// Replaces Delay_Ms to save linking overhead
void simple_delay(int cycles)
{
    while (cycles--)
    {
        __asm__("nop");
    }
}

// Replaces NVIC_SystemReset to save function call overhead
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
    case CMD_RESET:
        *BOOT_FLAG_ADDR = 0;
        raw_reset();
        break;
    case CMD_JUMP_BOOT:
        *BOOT_FLAG_ADDR = BOOT_MAGIC_VALUE;
        break;
    case CMD_SET_PTR:
        flash_pointer = 0x08000000 + (i2c_buffer[reg] | (i2c_buffer[reg + 1] << 8));
        break;
    case CMD_READ:
    {
        // Copy Flash -> Buffer so Master can read it
        uint8_t *src = (uint8_t *)flash_pointer;
        for (int i = 0; i < PAGE_SIZE; i++)
            i2c_buffer[reg + i] = src[i];
    }
    break;
    case CMD_WRITE:
        need_to_write = 1;
        break;
    case CMD_CHECK_ERR:
        // Optional: Clear error on write (stateless)
        // We handle error reporting by writing to the buffer in Main
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

    // 2. Configure LEDs (PD6, PA2, PC4)
    // PD6(24), PA2(8), PC4(16) -> Output 10MHz (0x1)
    GPIOD->CFGLR &= ~(0xF << 24);
    GPIOD->CFGLR |= (0x1 << 24);
    GPIOA->CFGLR &= ~(0xF << 8);
    GPIOA->CFGLR |= (0x1 << 8);
    GPIOC->CFGLR &= ~(0xF << 16);
    GPIOC->CFGLR |= (0x1 << 16);

    // 3. Configure I2C Pins (PC1, PC2) -> AF Open Drain (0xD)
    GPIOC->CFGLR &= ~(0xFF << 4);
    GPIOC->CFGLR |= (0xDD << 4);

    // 4. Sanity Blink (PD6)
    // ~200000 cycles approx 50ms at 48MHz (rough guess, timing not critical)
    for (int i = 0; i < 5; i++)
    {
        GPIOD->BSHR = (1 << 6);
        simple_delay(200000);
        GPIOD->BCR = (1 << 6);
        simple_delay(200000);
    }

    // 5. Init I2C (Pass NULL for Read Callback to save space)
    SetupI2CSlave(I2C_ADDR, i2c_buffer, sizeof(i2c_buffer), onWrite, NULL, false);

    // 6. Decision
    uint8_t stay = 0;
    if (*BOOT_FLAG_ADDR == BOOT_MAGIC_VALUE)
        stay = 1;
    *BOOT_FLAG_ADDR = 0;

    if (!stay)
    {
        simple_delay(2000000); // Wait approx 500ms
        if (*BOOT_FLAG_ADDR == BOOT_MAGIC_VALUE)
            stay = 1;
    }

    // 7. Jump Logic (If we are NOT staying, leave immediately)
    if (!stay)
    {
        // Cleanup LEDs
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

    // 8. Bootloader Loop (We are staying)
    GPIOA->BSHR = (1 << 2); // PA2 ON

    while (1)
    {
        if (need_to_write)
        {
            need_to_write = 0;
            GPIOC->BSHR = (1 << 4); // PC4 ON

            uint8_t *data_ptr = (uint8_t *)&i2c_buffer[CMD_WRITE];
            uint8_t recv_sum = i2c_buffer[CMD_WRITE + PAGE_SIZE];
            uint8_t calc_sum = 0;
            uint8_t err_code = 0;

            for (int i = 0; i < PAGE_SIZE; i++)
                calc_sum += data_ptr[i];

            if (calc_sum == recv_sum)
            {
                if (flash_pointer >= APP_START_ADDR)
                {
                    // INLINED FLASH WRITE
                    if (FLASH->CTLR & FLASH_CTLR_LOCK)
                    {
                        FLASH->KEYR = 0x45670123;
                        FLASH->KEYR = 0xCDEF89AB;
                    }

                    // Erase
                    FLASH->CTLR |= FLASH_CTLR_PER;
                    FLASH->ADDR = flash_pointer;
                    FLASH->CTLR |= FLASH_CTLR_STRT;
                    while (FLASH->STATR & FLASH_STATR_BSY)
                        ;
                    FLASH->CTLR &= ~FLASH_CTLR_PER;

                    // Program
                    FLASH->CTLR |= FLASH_CTLR_PG;
                    for (int i = 0; i < PAGE_SIZE; i += 2)
                    {
                        *(volatile uint16_t *)(flash_pointer + i) =
                            data_ptr[i] | (data_ptr[i + 1] << 8);
                        while (FLASH->STATR & FLASH_STATR_BSY)
                            ;
                    }
                    FLASH->CTLR &= ~FLASH_CTLR_PG;
                    FLASH->CTLR |= FLASH_CTLR_LOCK;

                    flash_pointer += PAGE_SIZE;
                    err_code = 0; // OK
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

            // UPDATE STATUS DIRECTLY IN BUFFER
            // Master will read this index (0x05) to check result
            i2c_buffer[CMD_CHECK_ERR] = err_code;

            GPIOC->BCR = (1 << 4); // PC4 OFF
        }
    }
}