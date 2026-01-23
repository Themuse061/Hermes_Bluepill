#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h> // For getch()

// Include your custom headers
#include "usb_serial.h"
#include "USB_commands.h"
#include "USB_commands_high.h"
#include "Command_ID.h"
#include "helper.h"
#include <bootloader_uploader.h>

// ============================================================================
// CROSS-PLATFORM SLEEP MACRO
// ============================================================================
#ifdef _WIN32
#include <windows.h>
#define SLEEP_MS(x) Sleep(x)
#else
#include <unistd.h>
#define SLEEP_MS(x) usleep((x) * 1000)
#endif

// ============================================================================
// CONFIGURATION
// ============================================================================
#define DEFAULT_I2C_ADDR 0x48
const char *COM_PORT = "COM14";
const int BAUD_RATE = 115200;

// ============================================================================
// DIAGNOSTIC FUNCTIONS
// ============================================================================

void print_hex_dump(const char *label, uint8_t *data, int len)
{
	printf("    %s: ", label);
	for (int i = 0; i < len; i++)
	{
		printf("%02X ", data[i]);
	}
	printf("\n");
}

void Test_Bootloader_Functionality(uint8_t target_addr)
{
	printf("\n--- STARTING DIAGNOSTICS ---\n");
	printf("Press ENTER to step through each test.\n");
	getch();

	// ---------------------------------------------------------
	// TEST 1: JUMP TO BOOTLOADER
	// ---------------------------------------------------------
	printf("\n[TEST 1] Sending Jump Command (0x01)...\n");
	printf("    -> Watch your Board! LED should switch to 'Bootloader Mode' (PA2 Solid).\n");

	// We call this manually to debug timing

	MCU_Jump_to_Bootloader(target_addr);

	printf("    Command sent. Waiting 600ms for reboot...\n");
	SLEEP_MS(600);
	printf("    [DONE] Press ENTER to check status...\n");
	getch();

	// ---------------------------------------------------------
	// TEST 2: CHECK ERROR REGISTER (Connectivity Check)
	// ---------------------------------------------------------
	printf("\n[TEST 2] Reading Error Register (0x05)...\n");
	// If the device is in bootloader, this should return 0 (ERR_OK)

	uint8_t err = MCU_Flash_Check_Error(target_addr);
	printf("    -> Error Code Received: 0x%02X\n", err);

	if (err == 0xFF)
		printf("    [WARNING] Possible I2C Timeout or NACK.\n");
	else if (err == 0x00)
		printf("    [PASS] Device responded with No Error.\n");
	else
		printf("    [INFO] Device responded with error state.\n");

	printf("    [DONE] Press ENTER to read Flash...\n");
	getch();

	// ---------------------------------------------------------
	// TEST 3: READ FLASH (Page 0 - Bootloader Area)
	// ---------------------------------------------------------
	printf("\n[TEST 3] Reading Flash Page 0 (0x0000)...\n");

	MCU_Flash_Set_Pointer(target_addr, 0x0000);

	uint8_t read_buf[64];
	memset(read_buf, 0x00, 64);

	MCU_Flash_Read_Page(target_addr, read_buf);
	print_hex_dump("Data", read_buf, 16); // Print first 16 bytes

	if (read_buf[0] == 0x00 && read_buf[1] == 0x00)
	{
		printf("    [WARN] Buffer is all Zeros. Read might have failed.\n");
	}
	else
	{
		printf("    [PASS] Data received.\n");
	}

	printf("    [DONE] Press ENTER to test Write Protection...\n");
	getch();

	// ---------------------------------------------------------
	// TEST 4: WRITE PROTECTION TEST
	// ---------------------------------------------------------
	printf("\n[TEST 4] Attempting to Write to Protected Area...\n");
	// We will try to write to Address 0x0000 (Bootloader).

	uint8_t dummy_data[64];
	memset(dummy_data, 0xAA, 64); // Fill with 0xAA pattern

	// 1. Set Pointer
	MCU_Flash_Set_Pointer(target_addr, 0x0000);

	// 2. Write
	printf("    Sending Write Command...\n");
	MCU_Flash_Write_Page(target_addr, dummy_data);

	// 3. Wait
	SLEEP_MS(100);

	// 4. Check Error
	err = MCU_Flash_Check_Error(target_addr);
	printf("    -> Result Error Code: 0x%02X\n", err);

	if (err == 0x02)
	{
		printf("    [PASS] Bootloader correctly returned ERR_ADDR (Protected).\n");
	}
	else if (err == 0x00)
	{
		printf("    [FAIL] Bootloader claimed Success (It shouldn't!).\n");
	}
	else
	{
		printf("    [FAIL] Unknown error code.\n");
	}

	printf("    [DONE] Press ENTER to Reset...\n");
	getch();

	// ---------------------------------------------------------
	// TEST 5: RESET
	// ---------------------------------------------------------
	printf("\n[TEST 5] Resetting MCU...\n");
	MCU_Reset(target_addr);
	printf("    Reset command sent.\n");
}

// ============================================================================
// MAIN ENTRY POINT
// ============================================================================
int main(int argc, char *argv[])
{
	printf("=========================================\n");
	printf("   CH32V003 I2C Diagnostics Tool         \n");
	printf("=========================================\n");

	// 1. Parse Arguments (Address only)
	uint8_t target_addr = DEFAULT_I2C_ADDR;
	if (argc >= 2)
	{
		unsigned int addr_in;
		if (sscanf(argv[1], "0x%x", &addr_in) == 1 || sscanf(argv[1], "%x", &addr_in) == 1)
		{
			target_addr = (uint8_t)addr_in;
		}
	}

	printf("[-] Configuration:\n");
	printf("    Address: 0x%02X\n", target_addr);
	printf("    Port:    %s\n", COM_PORT);

	// 2. Initialize USB Hardware
	printf("[-] Initializing USB Adapter...\n");
	if (USB_init(COM_PORT, BAUD_RATE) < 0)
	{
		fprintf(stderr, "Error: Failed to open port %s\n", COM_PORT);
		return 1;
	}

	// 3. Run Test Suite
	Test_Bootloader_Functionality(target_addr);

	// 4. Cleanup
	USB_deinit();

	printf("\nDiagnostics Complete. Press any key to exit.\n");
	getch();

	return 0;
}