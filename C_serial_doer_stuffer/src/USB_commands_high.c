#include "USB_commands_high.h"
#include "USB_commands.h"
#include <stdio.h>

#define TCA9554_ADDR 0x38
#define TCA9554_REG_OUTPUT 0x01
#define TCA9554_REG_CONFIG 0x03

#define CH32_CMD_GPIO_SET 0x10

void TCA9554_init(void)
{
    // 1. Configure all pins as outputs (Write 0x00 to Config Register 0x03)
    uint8_t config_cmd[] = {TCA9554_REG_CONFIG, 0x00};
    USB_command_i2c_write(TCA9554_ADDR, config_cmd, sizeof(config_cmd));

    // 2. Set initial output to 0 (Write 0x00 to Output Register 0x01)
    uint8_t output_cmd[] = {TCA9554_REG_OUTPUT, 0x00};
    USB_command_i2c_write(TCA9554_ADDR, output_cmd, sizeof(output_cmd));
}

void TCA9554_write(uint8_t data)
{
    // Write data to Output Register 0x01
    uint8_t output_cmd[] = {TCA9554_REG_OUTPUT, data};
    USB_command_i2c_write(TCA9554_ADDR, output_cmd, sizeof(output_cmd));
}

void ch32_small_expander_write(uint8_t addr, uint8_t data)
{
    // Mask data to lower 3 bits as requested
    uint8_t masked_data = data & 0x07;
    
    // Command format: [0x10, data]
    uint8_t cmd[] = {CH32_CMD_GPIO_SET, masked_data};
    
    USB_command_i2c_write(addr, cmd, sizeof(cmd));
}
