#include "USB_commands.h"
#include "usb_serial.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CMD_I2C_WRITE 0x01
#define CMD_I2C_SEND_RECV 0x02
#define CMD_ECHO 0x03
#define CMD_PING 0x04

static void log_packet(const char *cmd_name, const uint8_t *data, uint8_t len) {
    printf("-> MCU (%s)", cmd_name);
    for (int i = 0; i < len; i++) {
        printf(" %02X", data[i]);
    }
    printf("\n");
}

void USB_read_and_display(void) {
    unsigned char buffer[256];
    int bytes_read = USB_read(buffer, sizeof(buffer), 500); // 500ms timeout

    if (bytes_read > 0) {
        printf("MCU Response:");
        for (int i = 0; i < bytes_read; i++) {
            printf(" %02X", buffer[i]);
        }
        printf("\n");
    } else if (bytes_read == 0) {
        // Timeout
    } else {
        printf("MCU Response: Error reading port\n");
    }
}

void USB_command_ping(void) {
    // Packet: [Length, Command]
    // Length = 2 bytes total
    uint8_t packet[] = {0x02, CMD_PING};
    
    log_packet("Ping", packet, sizeof(packet));
    USB_write(packet, sizeof(packet));
    
    // Expect response
    USB_read_and_display();
}

void USB_command_echo(const uint8_t *data, uint8_t len) {
    // Packet: [Length, Command, Data...]
    // Length = 2 + len
    uint8_t packet_len = 2 + len;
    uint8_t *packet = (uint8_t *)malloc(packet_len);
    
    if (!packet) {
        fprintf(stderr, "Error: Malloc failed in USB_command_echo\n");
        return;
    }

    packet[0] = packet_len;
    packet[1] = CMD_ECHO;
    memcpy(&packet[2], data, len);

    log_packet("Echo", packet, packet_len);
    USB_write(packet, packet_len);
    
    // Expect response
    USB_read_and_display();

    free(packet);
}

void USB_command_i2c_write(uint8_t address, const uint8_t *data, uint8_t len) {
    // Packet: [Length, Command, Address, Data...]
    // Length = 3 + len
    uint8_t packet_len = 3 + len;
    uint8_t *packet = (uint8_t *)malloc(packet_len);

    if (!packet) {
        fprintf(stderr, "Error: Malloc failed in USB_command_i2c_write\n");
        return;
    }

    packet[0] = packet_len;
    packet[1] = CMD_I2C_WRITE;
    packet[2] = address;
    memcpy(&packet[3], data, len);

    log_packet("I2C_write", packet, packet_len);
    USB_write(packet, packet_len);

    // No response expected for I2C write (0x01)
    
    free(packet);
}
