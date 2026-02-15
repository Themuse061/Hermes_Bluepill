#pragma once
#include <stdint.h>
#include <stdbool.h>

// -------------------- CONFIGURATION BLOCK -------------------- //
// Parameters modulating work of the functions

// transmit/send buffer
#define HERMES_BUFFER_SEND_STACK_MAX_HEIGHT 16 // max amount of distictive commands in single transmision
#define HERMES_BUFFER_SEND_MAX_LENGTH 256	   // max lenght of single command

// recieve buffer
#define HERMES_BUFFER_RECIEVE_STACK_MAX_HEIGHT 16 // max amount of distictive commands in single transmision
#define HERMES_BUFFER_RECIEVE_MAX_LENGTH 256	  // max lenght of single command

#define HERMES_MIDDLEMAN_MAX_TIMEOUT_MS 10000 // time before middleman device (bluepill) is considered to be dead

// -------------------- USB BLOCK -------------------- //
// Functions for handling all stuff usb

int hermes_USB_init(const char *port_name, int baud_rate);
void hermes_USB_deinit(void);
int hermes_USB_send(const unsigned char *data, int length);
int hermes_USB_recieve(unsigned char *buffer, int length, unsigned int timeout_ms);
int hermes_USB_check_recieve_buffer(void);
int hermes_USB_wait_for_recieve(unsigned int max_delay_ms);

// -------------------- PACKET BLOCK -------------------- //
// Functions for programming at packet level

int hermes_packet_flush();			   // sends the buffor and waits for echo from middleman
int hermes_packet_flush_blind();	   // sends the buffor and does not waits for echo from middleman
int hermes_packet_check_height_free(); // returns how many empty spaces for commands are there in the stack buffer
int hermes_packet_check_heigh_taken(); // returns how many commands are in the stack buffer

// -------------------- ADD BLOCK -------------------- //
// Adding functions to command buffer

int hermes_add_I2C_write();
int hermes_add_I2C_send_recieve();
int hermes_add_echo();
int hermes_add_ping();
int hermes_add_delay_ms();
int hermes_add_I2C_read();

// -------------------- SEND BLOCK -------------------- //
// functions for handling entire stack_add-send-wait routine in single line if you don't care about speed

int hermes_send_I2C_write();
int hermes_send_I2C_send_recieve();
int hermes_send_echo();
int hermes_send_ping();
int hermes_send_delay_ms();
int hermes_send_I2C_read();

// -------------------- EASY BLOCK -------------------- //
// Functions that abstrac more than the above

int hermes_easy_reset_I2C(uint8_t addr);
int hermes_easy_jump_to_bootloader_I2C(uint8_t addr);
