#include <stdint.h>

#pragma once

/**
 * @brief Adds a single command packet to the internal stack.
 *
 * @param new_data Pointer to the byte array containing the command.
 * @param len Length of the command in bytes.
 * @return 1 on success, negative values on error:
 *         -1: Null data or zero length
 *         -2: Packet exceeds MAX_PACKET_WIDTH
 *         -3: Total buffer would overflow
 *         -4: Stack height limit reached
 */
int Hermes_Add_Command_To_Stack(uint8_t *new_data, uint8_t len);

int Hermes_Check_Stack_Length(void);

/**
 * @brief Returns the current number of packets in the stack.
 */
int Hermes_Check_Stack_Height(void);

int Hermes_Read_Buffer_USB(uint8_t *read_pointer, int len);

/**
 * @brief Sends all batched commands in the stack to the USB device.
 *
 * Clears the stack after sending.
 * @return Number of bytes sent on success, or error code from USB_write.
 */
int Hermes_Flush_Stack(void);

/**
 * @brief Sends the stack and appends an Echo command to verify the device is synced.
 *
 * This function waits for a response from the device.
 * @return 1 on success, negative values on error:
 *         -1: Echo response mismatch
 *         -4: Timeout or size mismatch
 *         -5: Write failed
 */
int Hermes_Flush_Stack_with_wait(void);

/**
 * @brief Sends the stack and reads a response from the device.
 *
 * @param read_data_buffer Buffer to store the read data.
 * @param len Expected length of data to read.
 * @return 1 on success, negative values on error:
 *         -4: Timeout or size mismatch
 *         -5: Write failed
 */
int Hermes_Flush_Stack_with_Read(uint8_t *read_data_buffer, uint8_t len);
