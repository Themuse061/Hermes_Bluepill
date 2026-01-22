#ifndef USB_COMMANDS_H
#define USB_COMMANDS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Sends a PING command (0x04) to the MCU and waits for a response.
 *
 * Logs the outgoing packet and the received response.
 * Expected Response: 09 04 FF aa 00 11 00 aa FF
 */
void USB_command_ping(void);

/**
 * @brief Sends an ECHO command (0x03) to the MCU and waits for the echoed response.
 *
 * @param data Pointer to the data to echo.
 * @param len Length of the data.
 */
void USB_command_echo(const uint8_t *data, uint8_t len);

/**
 * @brief Sends an I2C Write command (0x01) to the MCU.
 *
 * Does not expect a response from the MCU.
 *
 * @param address The 7-bit I2C address.
 * @param data Pointer to the data to write.
 * @param len Length of the data to write.
 */
void USB_command_i2c_write(uint8_t address, const uint8_t *data, uint8_t len);

/**
 * @brief Sends a Delay command (0x05) to the MCU.
 *
 * @param delay Delay in milliseconds (uint32_t).
 */
void USB_command_delay(uint32_t delay);

/**
 * @brief Reads pending data from the USB port and displays it (Legacy/Helper).
 * 
 * Used internally to consume and display responses.
 */
void USB_read_and_display(void);

#ifdef __cplusplus
}
#endif

#endif // USB_COMMANDS_H
