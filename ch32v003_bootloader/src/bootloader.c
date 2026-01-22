#include "ch32fun.h"
#include "i2c_slave.h"

// ===================================================================================
// CONFIGURATION
// ===================================================================================
#define I2C_ADDR 0x48
#define APP_START_ADDR 0x08000800 // Where Program B (Main App) lives
#define BOOT_FLAG_ADDR ((volatile uint32_t *)0x200007FC)
#define BOOT_MAGIC_VALUE 0xBEEFCAFE

// Command to trigger bootloader mode during the 500ms window
#define I2C_Slave_Command_Reset_MCU 0x00           // (universal)
#define I2C_Slave_Command_Jump_To_Bootloader 0x01  // (universal)
#define I2C_Slave_Command_Write_Flash 0x02         // (universal)
#define I2C_Slave_Command_Read_Flash 0x03          // (universal)
#define I2C_Slave_Command_Go_To_Flash_Address 0x04 // (universal)

volatile uint8_t i2c_buffer[64];

// ===================================================================================
// HELPER: Jump to Program B
// ===================================================================================
void JumpToApp(void)
{
    // 1. Disable Interrupts (Safety)
    __disable_irq();

    // 2. De-Init I2C (Important!)
    // We turn off the I2C peripheral so the Main App finds it in a clean reset state.
    // If we don't do this, the Main App's I2C init might fail or glitch.
    RCC->APB1PCENR &= ~(RCC_APB1Periph_I2C1);

    // 3. Relocate Vector Table
    // Tell the CPU that from now on, interrupts are handled by the Main App code
    // Mode 1 (Vectored) | Address
    asm volatile("csrw mtvec, %0" : : "r"(APP_START_ADDR | 1));

    // 4. Jump
    void (*app_entry)(void) = (void (*)(void))APP_START_ADDR;
    app_entry();

    while (1)
        ; // Should never reach here
}

// ===================================================================================
// I2C CALLBACK
// ===================================================================================
void onWrite(uint8_t reg, uint8_t length)
{
    switch (reg)
    {
    case I2C_Slave_Command_Jump_To_Bootloader:
        *BOOT_FLAG_ADDR = BOOT_MAGIC_VALUE;
        break;

    default:
        break;
    }

    // NOTE: If you are inside the "Bootloader Loop" later,
    // you would add handling here for Flash Write/Erase commands.
}

// ===================================================================================
// MAIN (The Bootloader)
// ===================================================================================
int main()
{
    SystemInit();
    funGpioInitAll();

    // 1. Initialize I2C
    funPinMode(PC1, GPIO_CFGLR_OUT_10Mhz_AF_OD); // SDA
    funPinMode(PC2, GPIO_CFGLR_OUT_10Mhz_AF_OD); // SCL
    SetupI2CSlave(I2C_ADDR, i2c_buffer, sizeof(i2c_buffer), onWrite, NULL, false);

    // 2. Reset the Flag
    // We clear it to ensure we only stay if we receive a *fresh* command
    // or if the Main App specifically reset us with the intention to update.
    // (If you want to support "Soft Reset from App", remove this line or check it before clearing)
    *BOOT_FLAG_ADDR = 0;

    // 3. Wait 500ms (Window of Opportunity)
    // During this delay, interrupts are active. If Master sends 0x01,
    // onWrite() will run and set *BOOT_FLAG_ADDR = BOOT_MAGIC_VALUE.
    Delay_Ms(500);

    // 4. The Decision
    if (*BOOT_FLAG_ADDR == BOOT_MAGIC_VALUE)
    {
        // --- BOOTLOADER LOOP ---
        // We received the command! Stay here and wait for firmware updates.

        // Visual confirmation (Optional: Turn on LED on PC4)
        funPinMode(PC4, GPIO_CFGLR_OUT_10Mhz_PP);
        funDigitalWrite(PC4, 1);

        while (1)
        {
            // This loop keeps the I2C Slave active.
            // You can add logic here to process Flash Write flags set by onWrite()
            // e.g., if(i2c_buffer[66] == 1) Flash_Erase...
        }
    }
    else
    {
        // --- JUMP TO MAIN APP ---
        // No command received. Go to Program B.
        JumpToApp();
    }
}