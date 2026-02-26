#pragma once

#define Test_ping_with_hermes_USB_sends 0
// Uses USB_drivers.c directly

#define Test_ping_with_Hermes_stack_and_manual_read 0
// Adds to stack, flushes and reads manually

#define Test_ping_with_Hermes_parse_USB 0
// Adds to stack, flushes and reads with parse USB

#define Test_ping_with_Hermes_parse_USB_multiple_pings 0
// Adds multiple pings to stack, flushes and reads with parse USB

#define Test_echo_add 0
// adds echo to stack and flushes blindly

#define Test_hermes_flush 0
// sends 5 pings and uses normal flush

#define Test_hermes_flush_long 0
// sends 15 pings and uses normal flush

#define test_send_ping 0
// Uses ping send instead of add

#define test_send_echo 0
// Uses echo send

#define TCA9554_ADDR 0x38
#define TCA9554_REG_OUTPUT 0x01
#define TCA9554_REG_CONFIG 0x03

#define Test_TCA9554_I2C_write 0
// Uses I2C write send

#define Test_CH32V003_I2C_write_and_delay 0
// Uses I2C write add and delay_ms add

#define Hot_plate_address 0x28
#define dummy1_ID 0x03
#define dummy2_ID 0x04
#define dummy3_ID 0x05
#define dummy1_len 4
#define dummy2_len 8
#define dummy3_len 3

#define Test_hot_plate_send_recieve 1
#define Test_hot_plate_send_and_read 0

/// To be implemented --------------------------------------
#define CH32V003_bootloader_jump_testing 0
#define Ch32V003_bootloader_jump_testing_addr 0x48
#define Ch32V003_bootloader_jump_testing_boot_addr 0x48

#define CH32V003_bootloader_get_version_testing 0
#define CH32V003_bootloader_get_version_testing_addr 0x48

#define FLASH_READ_SIZE 64
#define FLASH_PAGE_AMOUNT 256
#define Flash_Start 0x08000000

#define CH32V003_FLASH_read_testing 0
#define CH32V003_FLASH_read_testing_addr 0x48
