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
// FLASH HELPER FUNCTIONS
// ===================================================================================
void Flash_Unlock()
{
    if ((FLASH->CTLR & FLASH_CTLR_LOCK) != 0)
    {
        FLASH->KEYR = 0x45670123;
        FLASH->KEYR = 0xCDEF89AB;
    }
}

void Flash_ErasePage(uint32_t addr)
{
    Flash_Unlock();
    FLASH->CTLR |= FLASH_CTLR_PER;
    FLASH->ADDR = addr;
    FLASH->CTLR |= FLASH_CTLR_STRT;
    while (FLASH->STATR & FLASH_STATR_BSY)
        ;
    FLASH->CTLR &= ~FLASH_CTLR_PER;
    FLASH->CTLR |= FLASH_CTLR_LOCK; // Lock immediately after
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
    FLASH->CTLR |= FLASH_CTLR_LOCK; // Lock immediately after
}

// ===================================================================================
// I2C CALLBACKS
// ===================================================================================
void onWrite(uint8_t reg, uint8_t length)
{
    switch (reg)
    {
    case CMD_RESET:
        *BOOT_FLAG_ADDR = 0;
        NVIC_SystemReset();
        while (1)
            ;
        break;
    case CMD_JUMP_BOOT:
        *BOOT_FLAG_ADDR = BOOT_MAGIC_VALUE;
        break;
    case CMD_SET_PTR:
        flash_pointer = 0x08000000 + (i2c_buffer[reg] | (i2c_buffer[reg + 1] << 8));
        break;
    case CMD_READ:
    {
        uint8_t *src = (uint8_t *)flash_pointer;
        for (int i = 0; i < PAGE_SIZE; i++)
            i2c_buffer[reg + i] = src[i];
    }
    break;
    case CMD_WRITE:
        need_to_write = 1;
        break;
    case CMD_CHECK_ERR:
        last_error = 0;
        break;
    }
}

void onRead(uint8_t reg)
{
    if (reg == CMD_CHECK_ERR)
        i2c_buffer[reg] = last_error;
}

// ===================================================================================
// MAIN
// ===================================================================================
int main()
{
    SystemInit();

    // 1. Minimal GPIO Init (Only enable GPIOC for I2C)
    RCC->APB2PCENR |= RCC_APB2Periph_GPIOC;

    // 2. Direct Register Config for I2C Pins (PC1=SDA, PC2=SCL)
    // Mode: Multiplex Open Drain (11), Speed: 10MHz (01) -> 0xD
    // Clear bits 4-11 (PC1, PC2) and Set them to 0xDD
    GPIOC->CFGLR &= ~(0xFF << 4);
    GPIOC->CFGLR |= (0xDD << 4);

    // 3. Initialize I2C Slave
    SetupI2CSlave(I2C_ADDR, i2c_buffer, sizeof(i2c_buffer), onWrite, onRead, false);

    // 4. Decision Logic (Inlined)
    uint8_t stay = 0;

    // A: Soft Reset Check
    if (*BOOT_FLAG_ADDR == BOOT_MAGIC_VALUE)
        stay = 1;
    *BOOT_FLAG_ADDR = 0; // Clear immediately

    // B: Window of Opportunity Check
    if (!stay)
    {
        Delay_Ms(500);
        // If Command 0x01 arrived during Delay, onWrite sets flag to MAGIC
        if (*BOOT_FLAG_ADDR == BOOT_MAGIC_VALUE)
            stay = 1;
    }

    // 5. Execution
    if (stay)
    {
        // --- BOOTLOADER LOOP ---
        while (1)
        {
            if (need_to_write)
            {
                need_to_write = 0;

                // Pointers
                uint8_t *data_ptr = (uint8_t *)&i2c_buffer[CMD_WRITE];
                uint8_t recv_sum = i2c_buffer[CMD_WRITE + PAGE_SIZE];

                // Verify Checksum
                uint8_t calc_sum = 0;
                for (int i = 0; i < PAGE_SIZE; i++)
                    calc_sum += data_ptr[i];

                if (calc_sum == recv_sum)
                {
                    // Verify Address Protection
                    if (flash_pointer >= APP_START_ADDR)
                    {
                        Flash_ErasePage(flash_pointer);
                        Flash_ProgramPage(flash_pointer, data_ptr);
                        flash_pointer += PAGE_SIZE;
                        last_error = 0;
                    }
                    else
                    {
                        last_error = 2;
                    } // Protected
                }
                else
                {
                    last_error = 1;
                } // Bad Csum
            }
        }
    }
    else
    {
        // --- JUMP TO APP ---
        __disable_irq();
        RCC->APB1PCENR &= ~(RCC_APB1Periph_I2C1); // Disable I2C

        // Set Vector Table to App
        asm volatile("csrw mtvec, %0" : : "r"(APP_START_ADDR | 1));

        // Jump
        void (*app_entry)(void) = (void (*)(void))APP_START_ADDR;
        app_entry();
        while (1)
            ;
    }
}