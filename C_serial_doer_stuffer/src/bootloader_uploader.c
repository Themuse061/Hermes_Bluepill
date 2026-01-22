#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// Platform-specific sleep
#ifdef _WIN32
#include <windows.h>
#define SLEEP_MS(x) Sleep(x)
#else
#include <unistd.h>
#define SLEEP_MS(x) usleep((x) * 1000)
#endif

// ============================================================================
// PROTOCOL DEFINITIONS
// ============================================================================
#define PAGE_SIZE 64

// I2C Slave Commands
#define I2C_Slave_Command_Reset_MCU 0x00			 // (universal)
#define I2C_Slave_Command_Jump_To_Bootloader 0x01	 // (universal)
#define I2C_Slave_Command_Flash_Set_Pointer 0x02	 // (universal)
#define I2C_Slave_Command_Flash_Read_Page 0x03		 // (universal)
#define I2C_Slave_Command_Flash_Write_Page 0x04		 // (universal)
#define I2C_Slave_Command_Flash_Check_For_Error 0x05 // (universal)

// Error Codes (Returned by I2C_Slave_Command_Flash_Check_For_Error)
#define ERR_OK 0x00
#define ERR_CSUM 0x01
#define ERR_ADDR 0x02

// ============================================================================
// EXTERNAL DEPENDENCIES
// ============================================================================

// Provided by your USB library
extern void USB_command_i2c_write(uint8_t address, const uint8_t *data, uint8_t len);

// Modified prototype to include the output buffer
extern void USB_command_i2c_send_receive(uint8_t address, const uint8_t *write_data, uint8_t write_len, uint8_t read_len, uint8_t *read_buffer);

// ============================================================================
// PRIMITIVE FUNCTIONS
// ============================================================================

/**
 * @brief Sets the internal Flash Write Pointer on the MCU.
 */
void MCU_Flash_Set_Pointer(uint8_t i2c_addr, uint16_t offset)
{
	uint8_t pkt[3];
	pkt[0] = I2C_Slave_Command_Flash_Set_Pointer;
	pkt[1] = offset & 0xFF;		   // Low Byte
	pkt[2] = (offset >> 8) & 0xFF; // High Byte

	USB_command_i2c_write(i2c_addr, pkt, 3);
}

/**
 * @brief Checks the MCU status register for errors from the previous operation.
 */
uint8_t MCU_Flash_Check_Error(uint8_t i2c_addr)
{
	uint8_t cmd = I2C_Slave_Command_Flash_Check_For_Error;
	uint8_t response = 0xFF; // Default to unknown error

	// Write [0x05], then read 1 byte back into 'response'
	USB_command_i2c_send_receive(i2c_addr, &cmd, 1, 1, &response);

	return response;
}

/**
 * @brief Reads 64 bytes of Flash memory from the current Pointer location.
 */
void MCU_Flash_Read_Page(uint8_t i2c_addr, uint8_t *buffer)
{
	uint8_t cmd = I2C_Slave_Command_Flash_Read_Page;

	// Write [0x03], then read 64 bytes back into 'buffer'
	USB_command_i2c_send_receive(i2c_addr, &cmd, 1, PAGE_SIZE, buffer);
}

/**
 * @brief Sends 64 bytes of data + 1 calculated Checksum byte to the MCU.
 */
void MCU_Flash_Write_Page(uint8_t i2c_addr, const uint8_t *data)
{
	// Buffer: [CMD] + [64 Bytes Data] + [Checksum]
	uint8_t pkt[1 + PAGE_SIZE + 1];

	pkt[0] = I2C_Slave_Command_Flash_Write_Page;

	// Copy data
	memcpy(&pkt[1], data, PAGE_SIZE);

	// Calculate Checksum (Sum of 64 data bytes)
	uint8_t sum = 0;
	for (int i = 0; i < PAGE_SIZE; i++)
	{
		sum += data[i];
	}
	pkt[PAGE_SIZE + 1] = sum;

	USB_command_i2c_write(i2c_addr, pkt, sizeof(pkt));
}

// ============================================================================
// HIGH LEVEL UPLOADER
// ============================================================================

/**
 * @brief Flashes a binary file to the MCU via I2C.
 */
int MCU_Upload_Firmware(uint8_t i2c_addr, const char *filename)
{
	FILE *f = fopen(filename, "rb");
	if (!f)
	{
		printf("[-] Error: Cannot open file %s\n", filename);
		return -1;
	}

	// 1. Trigger Bootloader Entry
	printf("[-] Resetting to Bootloader...\n");
	uint8_t cmd_boot = I2C_Slave_Command_Jump_To_Bootloader;
	USB_command_i2c_write(i2c_addr, &cmd_boot, 1);

	// Send it twice to ensure we catch the window if logic is finicky
	SLEEP_MS(50);
	USB_command_i2c_write(i2c_addr, &cmd_boot, 1);

	// Wait for MCU to reboot and enter loop
	SLEEP_MS(600);

	// 2. Set Pointer to Start (Offset 0x0000 -> 0x08000800)
	printf("[-] Setting Write Pointer to 0x0000...\n");
	MCU_Flash_Set_Pointer(i2c_addr, 0x0000);

	uint8_t file_buf[PAGE_SIZE];
	size_t bytes_read;
	int page_index = 0;
	int error_count = 0;

	printf("[-] Flashing...\n");

	while ((bytes_read = fread(file_buf, 1, PAGE_SIZE, f)) > 0)
	{

		// Pad last partial page with 0xFF
		if (bytes_read < PAGE_SIZE)
		{
			memset(file_buf + bytes_read, 0xFF, PAGE_SIZE - bytes_read);
		}

		int retry = 0;
		int success = 0;

		while (retry < 3)
		{
			// A. Write Page
			MCU_Flash_Write_Page(i2c_addr, file_buf);

			// B. Wait for Flash Burn (Critical!)
			SLEEP_MS(15);

			// C. Verify Status
			uint8_t err = MCU_Flash_Check_Error(i2c_addr);

			if (err == ERR_OK)
			{
				success = 1;
				break; // Move to next page
			}
			else
			{
				printf("\n[!] Error on Page %d (Code: 0x%02X). Retry %d/3...\n", page_index, err, retry + 1);

				// If we failed, we must reset the pointer to the current page
				// because the auto-increment is only guaranteed on success.
				MCU_Flash_Set_Pointer(i2c_addr, page_index * PAGE_SIZE);
				retry++;
				SLEEP_MS(10);
			}
		}

		if (!success)
		{
			printf("\n[!] Failed to write Page %d after 3 retries. Aborting.\n", page_index);
			fclose(f);
			return -2; // Verification Error
		}

		page_index++;
		if (page_index % 8 == 0)
			printf(".");
		fflush(stdout);
	}

	printf("\n[-] Upload Complete (%d pages).\n", page_index);
	fclose(f);

	// 3. Reset to Application
	printf("[-] Rebooting to Application...\n");
	uint8_t cmd_reset = I2C_Slave_Command_Reset_MCU;
	USB_command_i2c_write(i2c_addr, &cmd_reset, 1);

	return 0; // Success
}