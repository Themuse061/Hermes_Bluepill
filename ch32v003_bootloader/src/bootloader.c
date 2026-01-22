#include "ch32fun.h"
#include "i2c_slave.h"

// ===================================================================================
// CONFIGURATION
// ===================================================================================
#define I2C_ADDR 0x48
#define APP_START_ADDR 0x00000800
#define BOOT_FLAG_ADDR ((volatile uint32_t *)0x200007FC)
#define BOOT_MAGIC_VALUE 0xBEEFCAFE

#define I2C_Slave_Command_Reset_MCU 0x00
#define I2C_Slave_Command_Jump_To_Bootloader 0x01

volatile uint8_t i2c_buffer[64];

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
    if (reg == I2C_Slave_Command_Reset_MCU)
    {
        NVIC_SystemReset();
        while (1)
            ;
    }
    else if (reg == I2C_Slave_Command_Jump_To_Bootloader)
    {
        *BOOT_FLAG_ADDR = BOOT_MAGIC_VALUE;
    }
}

// ===================================================================================
// MAIN (The Bootloader)
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
    SetupI2CSlave(I2C_ADDR, i2c_buffer, sizeof(i2c_buffer), onWrite, NULL, false);

    // 3. LOGIC FIX: Check if we came from the App BEFORE clearing the flag
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

    // 4. The Decision
    if (stay_in_bootloader)
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
            Delay_Ms(100);
        }
    }
    else
    {
        // Turn OFF debug LED before jumping
        funDigitalWrite(PD6, 0);

        // --- JUMP TO MAIN APP ---
        JumpToApp();
    }
}