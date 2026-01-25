#pragma once

#define Test_ping_with_USB_writes 0
// Uses USB_drivers.h
#define Test_ping_with_Hermes_stack_and_manual_read 0
// Adds to stack, flushes and reads
#define Test_ping_with_Hermes_flush_and_read 0
// Uses hermes flush and read
#define Test_echo 0
// Doesn't work properly lol

#define Test_TCA9554_I2C_write 0
// Uses I2C write and command packing
#define Test_CH32V003_I2C_write_and_delay 1
// Uses I2C write, command packing and delay
#define TCA9554_ADDR 0x38
#define TCA9554_REG_OUTPUT 0x01
#define TCA9554_REG_CONFIG 0x03

#define Hot_plate_is_connected 1
#define Hot_plate_address 0x28

#define CH32V003_gpio_expander_is_conected 0
#define CH32V003_gpio_expander_address 0x09
