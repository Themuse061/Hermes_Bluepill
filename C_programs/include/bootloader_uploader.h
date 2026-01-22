#pragma once

#include <stdint.h>
#include <stdio.h>

/**
 * @brief Sets the internal Flash Write Pointer on the MCU.
 *
 * @param i2c_addr  The target MCU address.
 * @param offset    Address offset (e.g., 0x0000 for start of App).
 */
void MCU_Flash_Set_Pointer(uint8_t i2c_addr, uint16_t offset);

/**
 * @brief Checks the MCU status register for errors from the previous operation.
 *
 * @param i2c_addr  The target MCU address.
 * @return uint8_t  0x00 = OK, 0x01 = Checksum Err, 0x02 = Addr Err.
 */
uint8_t MCU_Flash_Check_Error(uint8_t i2c_addr);

/**
 * @brief Sends 64 bytes of data + 1 calculated Checksum byte to the MCU.
 *
 * @param i2c_addr  The target MCU address.
 * @param data      Pointer to the 64-byte data chunk.
 */
void MCU_Flash_Write_Page(uint8_t i2c_addr, const uint8_t *data);

/**
 * @brief Reads 64 bytes of Flash memory from the current Pointer location.
 *
 * @param i2c_addr  The target MCU address.
 * @param buffer    Pointer to a buffer (must be at least 64 bytes) to store the result.
 */
void MCU_Flash_Read_Page(uint8_t i2c_addr, uint8_t *buffer);

/**
 * @brief Flashes a binary file to the MCU via I2C.
 *
 * Sequence:
 * 1. Resets MCU into Bootloader.
 * 2. Sets Pointer to 0.
 * 3. Iterates through file in 64-byte chunks.
 * 4. Writes Page -> Waits -> Checks Error.
 * 5. Resets MCU to Application.
 *
 * @param i2c_addr   The target MCU address.
 * @param filename   Path to the .bin file.
 * @return int       0 on Success, -1 on File Error, -2 on Verification Error.
 */
int MCU_Upload_Firmware(uint8_t i2c_addr, const char *filename);