#include "USB_commands.h"
#include "usb_serial.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define I2C_Slave_Command_Reset_MCU 0x00           // (universal)
#define I2C_Slave_Command_Jump_To_Bootloader 0x01  // (universal)
#define I2C_Slave_Command_Write_Flash 0x02         // (universal)
#define I2C_Slave_Command_Read_Flash 0x03          // (universal)
#define I2C_Slave_Command_Go_To_Flash_Address 0x04 // (universal)

#define I2C_Slave_Command_GPIO_Set 0x10 // (dark_times)

// USB Device Commands
#define USB_Device_Command_I2C_Write 0x01
#define USB_Device_Command_I2C_Send_Receive 0x02
#define USB_Device_Command_Echo 0x03
#define CMD_PING 0x04
#define USB_Device_Command_Delay_Ms 0x05

#define USB_Device_Command_PC_Short_Data_Return 0xFF

static void log_packet(const char *cmd_name, const uint8_t *data, uint8_t len)
{
    printf("-> MCU (%s)", cmd_name);
    for (int i = 0; i < len; i++)
    {
        printf(" %02X", data[i]);
    }
    printf("\n");
}

void USB_read_and_display(void)
{
    unsigned char buffer[256];
    int bytes_read = USB_read(buffer, sizeof(buffer), 500); // 500ms timeout

    if (bytes_read > 0)
    {
        printf("MCU Response:");
        for (int i = 0; i < bytes_read; i++)
        {
            printf(" %02X", buffer[i]);
        }
        printf("\n");
    }
    else if (bytes_read == 0)
    {
        // Timeout
    }
    else
    {
        printf("MCU Response: Error reading port\n");
    }
}

void USB_command_ping(void)
{
    // Packet: [Length, Command]
    // Length = 2 bytes total
    uint8_t packet[] = {0x02, CMD_PING};

    log_packet("Ping", packet, sizeof(packet));
    USB_write(packet, sizeof(packet));

    // Expect response
    USB_read_and_display();
}

void USB_command_echo(const uint8_t *data, uint8_t len)
{
    // Packet: [Length, Command, Data...]
    // Length = 2 + len
    uint8_t packet_len = 2 + len;
    uint8_t *packet = (uint8_t *)malloc(packet_len);

    if (!packet)
    {
        fprintf(stderr, "Error: Malloc failed in USB_command_echo\n");
        return;
    }

    packet[0] = packet_len;
    packet[1] = USB_Device_Command_Echo;
    memcpy(&packet[2], data, len);

    log_packet("Echo", packet, packet_len);
    USB_write(packet, packet_len);

    // Expect response
    USB_read_and_display();

    free(packet);
}

void USB_command_i2c_write(uint8_t address, const uint8_t *data, uint8_t len)
{
    // Packet: [Length, Command, Address, Data...]
    // Length = 3 + len
    uint8_t packet_len = 3 + len;
    uint8_t *packet = (uint8_t *)malloc(packet_len);

    if (!packet)
    {
        fprintf(stderr, "Error: Malloc failed in USB_command_i2c_write\n");
        return;
    }

    packet[0] = packet_len;
    packet[1] = USB_Device_Command_I2C_Write;
    packet[2] = address;
    memcpy(&packet[3], data, len);

    log_packet("I2C_write", packet, packet_len);
    USB_write(packet, packet_len);

    // No response expected for I2C write (0x01)

    free(packet);
}

void USB_command_i2c_send_receive(uint8_t address, const uint8_t *write_data, uint8_t write_len, uint8_t read_len, uint8_t *read_buffer)
{
    // Packet: [Length, Command, Address, WriteLength, ReadLength, WriteData...]
    // Length = 5 + write_len
    uint8_t packet_len = 5 + write_len;
    uint8_t *packet = (uint8_t *)malloc(packet_len);

    if (!packet)
    {
        fprintf(stderr, "Error: Malloc failed in USB_command_i2c_send_receive\n");
        return;
    }

    packet[0] = packet_len;
    packet[1] = USB_Device_Command_I2C_Send_Receive;
    packet[2] = address;
    packet[3] = write_len;
    packet[4] = read_len;
    memcpy(&packet[5], write_data, write_len);

    log_packet("I2C_send_receive", packet, packet_len);
    USB_write(packet, packet_len);

    // Expect response:
    // uint8_t[0] - Packet Length
    // uint8_t[1] - USB_Device_Command_PC_Short_Data_Return (0xFF)
    // uint8_t[2] - USB_Device_Command_I2C_Send_Receive (0x02)
    // uint8_t[3] - Address
    // uint8_t[4+] - returned data

    unsigned char buffer[256];
    int bytes_read = USB_read(buffer, sizeof(buffer), 500); // 500ms timeout

    if (bytes_read > 0)
    {
        // Simple validation
        if (buffer[1] == USB_Device_Command_PC_Short_Data_Return && buffer[2] == USB_Device_Command_I2C_Send_Receive)
        {
             // Copy data to user buffer
             // Data starts at index 4
             int data_len_in_packet = bytes_read - 4;
             if (data_len_in_packet >= read_len) {
                 memcpy(read_buffer, &buffer[4], read_len);
             } else {
                 fprintf(stderr, "Warning: Received less data than expected. Req: %d, Rec: %d\n", read_len, data_len_in_packet);
                 if (data_len_in_packet > 0) {
                    memcpy(read_buffer, &buffer[4], data_len_in_packet);
                 }
             }
        }
        else
        {
             printf("MCU Response (Unexpected):");
             for (int i = 0; i < bytes_read; i++)
             {
                 printf(" %02X", buffer[i]);
             }
             printf("\n");
        }
    }
    else if (bytes_read == 0)
    {
        fprintf(stderr, "Timeout waiting for response in I2C_send_receive\n");
    }
    else
    {
        fprintf(stderr, "Error reading port in I2C_send_receive\n");
    }

    free(packet);
}

void USB_command_delay(uint32_t delay)
{
    // Packet: [Length, Command, Delay(4 bytes)]
    // Length = 2 + 4 = 6
    uint8_t packet_len = 6;
    uint8_t packet[6];

    packet[0] = packet_len;
    packet[1] = USB_Device_Command_Delay_Ms;
    packet[2] = delay & 0xFF;
    packet[3] = (delay >> 8) & 0xFF;
    packet[4] = (delay >> 16) & 0xFF;
    packet[5] = (delay >> 24) & 0xFF;

    log_packet("Delay", packet, packet_len);
    USB_write(packet, packet_len);
}
