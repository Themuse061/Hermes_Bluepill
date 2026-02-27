#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

// -------------------- VERVOSITY SETTINGS -------------------- //
// How much of printf'ing the functions do
// 0 - errors, always printing
// 1 - warnings
// 2 - verbose, each function does a print
// 3 - dump, raw data dumps with each send and recieve

#define HERMES_VERBOSITY_USB 1
// opening ports, sending packets, reading buffer

#define HERMES_PACKET_VERBOSITY 1

#define HERMES_ADD_VERBOSITY_USB 1

#define HERMES_SEND_VERBOSITY 1

#define HERMES_EASY_VERBOSITY 1

/*
print template
printf("-LOG- [level] [category], [function]: [description]\n");
*/

// -------------------- CONFIGURATION BLOCK -------------------- //
// Parameters modulating work of the functions

// transmit/send buffer
#define HERMES_BUFFER_SEND_STACK_MAX_HEIGHT 16 // max amount of distictive commands in single transmision.
#define HERMES_BUFFER_SEND_MAX_LENGTH 256	   // max lenght of single command

// recieve buffer
#define HERMES_BUFFER_RECIEVE_STACK_MAX_HEIGHT 16 // max amount of distictive commands in single transmision
#define HERMES_BUFFER_RECIEVE_MAX_LENGTH 256	  // max lenght of single command
#define HERMES_BUFFER_MAX_BYTES (16 * 256)		  // max length of temporary buffer for direct USB reads/writes

#define HERMES_MIDDLEMAN_MAX_TIMEOUT_MS 10000 // time before middleman device (bluepill) is considered to be dead

// -------------------- EXTERN VARIABLES BLOCK -------------------- //
// Declared in hermes_packet.c

extern uint8_t hermes_send_buffer[HERMES_BUFFER_SEND_STACK_MAX_HEIGHT][HERMES_BUFFER_SEND_MAX_LENGTH];
extern uint8_t hermes_recieve_buffer[HERMES_BUFFER_RECIEVE_STACK_MAX_HEIGHT][HERMES_BUFFER_RECIEVE_MAX_LENGTH];

// ------------------------------------------------------------------------------------------------ //
// ---------------------------------------- FUNCTION START ---------------------------------------- //
// ------------------------------------------------------------------------------------------------ //

// -------------------- USB BLOCK -------------------- //
// Functions for handling all stuff usb

int hermes_USB_init(const char *port_name, int baud_rate);
void hermes_USB_deinit(void);
int hermes_USB_send(const unsigned char *data, int length);
int hermes_USB_recieve(unsigned char *buffer, int length);
int hermes_USB_recieve_with_timeout(unsigned char *buffer, int length, unsigned int timeout_ms);
int hermes_USB_check_recieve_buffer(void);
int hermes_USB_wait_for_recieve(unsigned int max_delay_ms);

// -------------------- PACKET BLOCK -------------------- //
// Functions for programming at packet level

/// @brief
/// sends the buffor and does not waits for echo from middleman
int hermes_packet_flush_blind();

/// @brief
/// reads the usb buffer and parses the data into recieve buffer You need to wait for all the data by yourself
int hermes_packet_parse_USB();

/// @brief
/// sends the buffer and waits for echo from middleman. Leaves data in hermes_recieve_buffer
int hermes_packet_flush();

/// @brief
/// returns how many empty spaces for commands are there in the stack buffer
int hermes_packet_check_height_free();

/// @brief
/// returns how many commands are in the stack buffer
int hermes_packet_check_heigh_taken();

/// @brief
/// adds a command to command buffer
int hermes_packet_add_comand(uint8_t *command_array, int len);

int hermes_packet_add_comand_without_advancing_stack_height(uint8_t *command_array, int len);

// -------------------- ADD BLOCK -------------------- //
// Adding functions to command buffer

int hermes_add_I2C_write(uint8_t I2C_address, uint8_t *data, uint8_t len);
int hermes_add_I2C_send_recieve(uint8_t I2C_address, uint8_t write_len, uint8_t read_len, uint8_t *write_data);
int hermes_add_echo(uint8_t *data, uint8_t len);
int hermes_add_ping(void);
int hermes_add_delay_ms(int delay);
int hermes_add_I2C_read(uint8_t addr, uint8_t len);

// -------------------- SEND BLOCK -------------------- //
// functions for handling entire stack_add-send-wait routine in single line if you don't care about speed

int hermes_send_I2C_write(uint8_t I2C_address, uint8_t *data, uint8_t len);
int hermes_send_I2C_send_recieve(uint8_t I2C_address, uint8_t write_len, uint8_t read_len, uint8_t *write_data);
int hermes_send_echo(uint8_t *data, uint8_t len);
int hermes_send_ping();
int hermes_send_delay_ms(int delay);
int hermes_send_I2C_read(uint8_t addr, uint8_t len);

// -------------------- EASY BLOCK -------------------- //
// Functions that abstrac more than the above

/// @brief
/// Sends a I2C write with Command_ID_I2C_Slave_Reset_MCU
int hermes_easy_I2C_reset(uint8_t addr);

/// @brief
/// Sends a I2C write with Command_ID_I2C_Slave_Jump_To_Bootloader
int hermes_easy_I2C_jump_to_bootloader(uint8_t addr);

int hermes_easy_I2C_add_send_flash_pointer(uint8_t addr, uint32_t pointer);
int hermes_easy_I2C_add_read_flash(uint8_t addr, int amount);
int hermes_easy_i2C_add_write_flash_64_bytes(uint8_t addr, uint8_t *data);