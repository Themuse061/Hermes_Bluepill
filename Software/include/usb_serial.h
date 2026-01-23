/**
 * @file usb_serial.h
 * @brief USB serial port abstraction layer.
 */

#ifndef USB_SERIAL_H
#define USB_SERIAL_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the USB serial port.
 *
 * Finds and opens the specified serial port, and configures it with the given baud rate.
 * Configures 8 data bits, no parity, 1 stop bit (8N1).
 *
 * @param port_name Name of the serial port (e.g., "COM14", "/dev/ttyUSB0").
 * @param baud_rate Baud rate (e.g., 115200).
 * @return 0 on success, negative error code on failure.
 */
int USB_init(const char *port_name, int baud_rate);

/**
 * @brief De-initialize the USB serial port.
 *
 * Closes the port and frees resources.
 */
void USB_deinit(void);

/**
 * @brief Write data to the serial port.
 *
 * Blocks until data is passed to the OS.
 *
 * @param data Pointer to the data to write.
 * @param length Number of bytes to write.
 * @return Number of bytes written on success, negative error code on failure.
 */
int USB_write(const unsigned char *data, int length);

/**
 * @brief Read data from the serial port.
 *
 * Blocks until the requested number of bytes are read or timeout occurs.
 *
 * @param buffer Buffer to store read data.
 * @param length Maximum number of bytes to read.
 * @param timeout_ms Timeout in milliseconds (0 for infinite).
 * @return Number of bytes read on success, negative error code on failure.
 */
int USB_read(unsigned char *buffer, int length, unsigned int timeout_ms);

/**
 * @brief Check how much data is available to read.
 *
 * @return Number of bytes waiting in the input buffer, or negative error code on failure.
 */
int USB_check_for_data(void);

/**
 * @brief Wait for data to become available.
 *
 * Blocks until data is available to read or the timeout expires.
 *
 * @param max_delay_ms Maximum delay in milliseconds (0 for infinite).
 * @return 0 if data is available (or timeout occurred), negative error code on system error.
 */
int USB_wait_for_data(unsigned int max_delay_ms);

#ifdef __cplusplus
}
#endif

#endif // USB_SERIAL_H
