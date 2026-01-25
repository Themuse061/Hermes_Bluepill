#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hermes_packet_sending.h"
#include "Command_ID.h"

#define USB_COMMANDS_VERBOSE_LEVEL 1 // 0 for errors, 1 for normal, 2 for verbose

int Stack_add_ping(void)
{
    uint8_t packet[] = {2, Command_ID_USB_Device_Ping};

    if (USB_COMMANDS_VERBOSE_LEVEL > 1)
    {
        printf("LOG 2: Adding ping to stack\n");
    }

    return Hermes_Add_Command_To_Stack(packet, sizeof(packet));
}

// #define Command_ID_USB_Device_I2C_Write 0x01
// #define Command_ID_USB_Device_I2C_Send_Receive 0x02
// #define Command_ID_USB_Device_Echo 0x03
// #define Command_ID_USB_Device_Ping 0x04
// #define Command_ID_USB_Device_Delay_Ms 0x05

/**
 * @brief Adds data to echo to buffer
 *
 * @param data ONLY DATA NO COMMAND NUMBER
 */
int Stack_add_echo(uint8_t *data, uint8_t len)
{

    uint8_t echo_header[] = {len + 2, Command_ID_USB_Device_Echo};

    Hermes_Add_Command_To_Stack_Withouta_Advancing_The_Stack_Height(echo_header, 2);

    if (USB_COMMANDS_VERBOSE_LEVEL > 1)
    {
        printf("LOG 2: Adding echo to stack\n");
    }
    return Hermes_Add_Command_To_Stack(data, len);
}

int Stack_add_I2C_Write(uint8_t I2C_address, uint8_t *data, uint8_t len)
{
    uint8_t I2C_header[] = {len + 3, Command_ID_USB_Device_I2C_Write, I2C_address};
    Hermes_Add_Command_To_Stack_Withouta_Advancing_The_Stack_Height(I2C_header, 3);

    if (USB_COMMANDS_VERBOSE_LEVEL > 1)
    {
        printf("LOG 2: Adding I2C Write to stack\n");
    }
    return Hermes_Add_Command_To_Stack(data, len);
}

int Stack_add_delay(int delay)
{

    uint8_t packet[] = {
        6,                              // Total packet length
        Command_ID_USB_Device_Delay_Ms, // Command ID
        (uint8_t)(delay & 0xFF),        // LSB
        (uint8_t)((delay >> 8) & 0xFF),
        (uint8_t)((delay >> 16) & 0xFF),
        (uint8_t)((delay >> 24) & 0xFF) // MSB
    };

    if (USB_COMMANDS_VERBOSE_LEVEL > 1)
    {
        printf("LOG 2: Adding delay to stack: %d ms\n", delay);
    }

    return Hermes_Add_Command_To_Stack(packet, sizeof(packet));
}