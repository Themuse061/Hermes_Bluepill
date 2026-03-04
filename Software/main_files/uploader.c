#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "hermes_header.h"

#define DO_DEBUG_PRINTS 1
const char *binary_path = NULL;
#define FULL_UINT64 0xFFFFFFFFFFFFFFFF
uint64_t flash_start = FULL_UINT64;
uint64_t flash_size = FULL_UINT64;
int start_at_page = 0;
int page_size = 64;
int I2C_main_addr = 0x48;
int I2C_bootloader_addr = 0x48;
const char *COM_PORT = "COM14";
const int BAUD_RATE = 115200;

FILE *binary_file = NULL;
bool is_hermes_connected = 0;

void exit_gracefully(int exit_code, const char *error_message)
{
	printf(error_message);
	if (binary_file != NULL)
	{
		fclose(binary_file);
	}

	if (is_hermes_connected)
	{
		hermes_USB_deinit();
	}

	exit(exit_code);
}

// ---------- MAIN ---------- //
int main(int argc, char const *argv[])
{

	// disable printf buffering
	setvbuf(stdout, NULL, _IONBF, 0);

	// Parse arguments
	for (int i = 1; i < argc; i++) // start from first argument
	{
		// if we find --help
		if (strcmp(argv[i], "--help") == 0)
		{
			printf("Avaliable parameters:\n");
			printf("--path \"./path/to/binary.bin\"\n");
			printf("--flash_start 0x123456 (Where to start the read)\n");
			printf("--flash_size 0x123456 (How big the write can be)\n");
			printf("--page_size 123 (Default is 64)\n");
			printf("--I2C_main_addr 0xaddr (What I2C addres to reset when jumping to bootloader, default 0x48)\n");
			printf("--I2C_bootloader_addr 0xaddr (What addres to talk to when flashing, default 0x48)\n");
			printf("--start_at_page 1234 (At which page to start programming, default 0)\n");
			return 0;
		}
		else if (strcmp(argv[i], "--path") == 0) // if we find --path
		{
			binary_path = argv[i + 1];
		}
		else if (strcmp(argv[i], "--flash_start") == 0) // if we find --flash_start
		{
			flash_start = strtoul(argv[i + 1], NULL, 0);
		}
		else if (strcmp(argv[i], "--flash_size") == 0) // if we find --flash_size
		{
			flash_size = strtoul(argv[i + 1], NULL, 0);
		}
		else if (strcmp(argv[i], "--page_size") == 0) // if we find --page_size
		{
			page_size = strtoul(argv[i + 1], NULL, 0);
		}
		else if (strcmp(argv[i], "--I2C_main_addr") == 0) // if we find --I2C_main_addr
		{
			I2C_main_addr = strtoul(argv[i + 1], NULL, 0);
		}
		else if (strcmp(argv[i], "--I2C_bootloader_addr") == 0) // if we find --I2C_bootloader_addr
		{
			I2C_bootloader_addr = strtoul(argv[i + 1], NULL, 0);
		}
		else if (strcmp(argv[i], "--start_at_page") == 0) // if we find --start_at_page
		{
			start_at_page = strtoul(argv[i + 1], NULL, 0);
		}
	}

	if (DO_DEBUG_PRINTS)
	{
		printf("\n----- DEBUG ----- \n");
		printf("Finished parsing parameters from cmd\n");
		printf("Path is %s\n", binary_path);
		printf("Flash start is 0x%" PRIx64 "\n", flash_start);
		printf("Flash size is  0x%" PRIX64 "\n", flash_size);
		printf("Page size is %i\n", page_size);
		printf("I2C main addr is 0x%02x\n", I2C_main_addr);
		printf("I2C bootloader addr is 0x%02X\n", I2C_bootloader_addr);
		printf("----- DEBUG ----- \n\n");
	}

	// check if user has specified the important things
	if (binary_path == NULL)
	{
		exit_gracefully(1, "ERROR: You need to specify binary path!");
	}

	if (flash_start == FULL_UINT64)
	{

		exit_gracefully(1, "ERROR: You need to flash start address!");
	}

	if (flash_size == FULL_UINT64)
	{

		exit_gracefully(1, "ERROR: You need to specify flash size!");
	}

	binary_file = fopen(binary_path, "rb");

	if (binary_file == NULL)
	{
		exit_gracefully(1, "error opening file!");
	}

	if (DO_DEBUG_PRINTS)
	{
		uint8_t buffer[64];
		size_t bytes_read;
		size_t total_offset = 0;

		printf("\n----- DEBUG ----- \n");
		printf("File opened successfully.\n");
		printf("The contents are:");
		for (int i = 0; i < 2; i++)
		{
			bytes_read = fread(buffer, 1, sizeof(buffer), binary_file);
			// Print each chunk
			for (size_t i = 0; i < bytes_read; i++)
			{
				// Print address every 16 bytes (or at the start of the line)
				if (i % 16 == 0)
				{
					printf("\n0x%08zX: ", total_offset + i);
				}

				// Print the byte in hex format
				printf("%02X ", buffer[i]);
			}

			total_offset += bytes_read;
		}
		rewind(binary_file);

		printf("\n----- DEBUG ----- \n\n");
	}

	int all_pages = flash_size / page_size;
	if ((flash_size % page_size) != 0)
	{
		exit_gracefully(1, "flash_size isn't multiply of page_size\n exiting...");
	}

	printf("Starting Programming!\n");

	uint32_t flash_pointer = flash_start + start_at_page * page_size;

	if (hermes_USB_init(COM_PORT, BAUD_RATE) < 0)
	{
		exit_gracefully(1, "Cant connect to Hermes");
	}
	is_hermes_connected = 1;

	printf("Jumping to bootloader...\n");
	hermes_easy_I2C_jump_to_bootloader(I2C_main_addr, I2C_bootloader_addr);

	// skip start_at_page pages
	if (fseek(binary_file, start_at_page * page_size, SEEK_SET) != 0)
	{
		exit_gracefully(1, "Failed at skipping to offset");
	}
	uint8_t page_buffer[page_size];

	while (flash_pointer < (flash_start + flash_size))
	{
		int current_page = (flash_pointer - flash_start) / page_size;
		printf("wrting page %d/%d\n", current_page, all_pages);

		size_t bytes_read = fread(page_buffer, 1, page_size, binary_file);
		if (bytes_read < (size_t)page_size)
		{
			if (feof(binary_file))
			{
				printf("Reached end of file, exiting write loop\n");
				break;
			}
			else if (ferror(binary_file))
			{
				exit_gracefully(1, "An error occurred when reading.\n");
			}
		}
		hermes_easy_I2C_add_send_flash_pointer(I2C_bootloader_addr, flash_pointer);
		hermes_easy_i2C_add_write_flash_64_bytes(I2C_bootloader_addr, page_buffer);
		hermes_packet_flush();

		flash_pointer += page_size;
	}

	hermes_easy_I2C_reset(I2C_bootloader_addr);

	exit_gracefully(0, "everything good. exiting...");
}