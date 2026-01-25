#pragma once

#define Test_ping_with_USB_writes 0
#define Test_ping_with_Hermes_stack_and_manual_read 0
#define Test_ping_with_Hermes_flush_and_read 0
#define Test_echo 1

#define TCA9554_REG_OUTPUT 0x01
#define TCA9554_REG_CONFIG 0x03

#define TCA9554_is_connected 1
#define TCA9554_is_initialized 1
#define Hot_plate_is_connected 1
#define CH32V003_gpio_expander_is_conected 0

#define TCA9554_ADDR 0x38 // not used
#define Hot_plate_address 0x28
#define CH32V003_gpio_expander_address 0x09