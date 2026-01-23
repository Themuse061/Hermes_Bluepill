#ifndef USB_COMMANDS_HIGH_H
#define USB_COMMANDS_HIGH_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes the TCA9554 GPIO expander.
 * 
 * Sets the configuration register (0x03) to 0x00 (all outputs)
 * and clears the output register (0x01) to 0x00.
 * Uses I2C address 0x38.
 */
void TCA9554_init(void);

/**
 * @brief Writes to the TCA9554 output register.
 * 
 * Writes the given data byte to register 0x01 (Output Port Register).
 * Uses I2C address 0x38.
 * 
 * @param data The 8-bit data to write to the output port.
 */
void TCA9554_write(uint8_t data);

/**
 * @brief Writes to the ch32v003 small expander.
 * 
 * Sends a command to the specified I2C address with command 0x10 (GPIO Set).
 * Only the lower 3 bits of the data are used (0b00000xxx).
 * 
 * @param addr The 7-bit I2C address of the ch32 slave (e.g., 0x09).
 * @param data The data to write (masked to 3 bits).
 */
void ch32_small_expander_write(uint8_t addr, uint8_t data);

#ifdef __cplusplus
}
#endif

#endif // USB_COMMANDS_HIGH_H
