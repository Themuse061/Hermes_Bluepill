#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hermes_packet_sending.h"
#include "Command_ID.h"

int hermes_add_I2C_write(uint8_t I2C_address, uint8_t *data, uint8_t len)
{
    uint8_t I2C_header[] = {len + 3, Command_ID_USB_Device_I2C_Write, I2C_address};
    Hermes_Add_Command_To_Stack_Withouta_Advancing_The_Stack_Height(I2C_header, 3);

    if (HERMES_VERBOSITY_USB > 1)
    {
        printf("LOG 2: Adding I2C Write to stack\n");
    }
    return Hermes_Add_Command_To_Stack(data, len);
}
/**
 * @brief YOU NEEED TO HANDLE RECIEVE
 */
int hermes_add_I2C_send_recieve(uint8_t I2C_address, uint8_t write_len, uint8_t read_len, uint8_t *write_data)
{
    uint8_t command_len = 5 + write_len;
    uint8_t Send_Recieve_command[] = {
        command_len, Command_ID_USB_Device_I2C_Send_Receive,
        I2C_address, write_len, read_len};

    if (HERMES_VERBOSITY_USB > 1)
    {
        printf("LOG 2: Adding I2C Send Recieve to stack\n");
    }
    Hermes_Add_Command_To_Stack_Withouta_Advancing_The_Stack_Height(Send_Recieve_command, 5);
    return Hermes_Add_Command_To_Stack(write_data, write_len);
}

/**
 * @brief Adds data to echo to buffer
 *
 * @param data ONLY DATA NO COMMAND NUMBER
 */
int hermes_add_echo(uint8_t *data, uint8_t len)
{

    uint8_t echo_header[] = {len + 2, Command_ID_USB_Device_Echo};

    Hermes_Add_Command_To_Stack_Withouta_Advancing_The_Stack_Height(echo_header, 2);

    if (HERMES_VERBOSITY_USB > 1)
    {
        printf("LOG 2: Adding echo to stack\n");
    }
    return Hermes_Add_Command_To_Stack(data, len);
}

int hermes_add_ping(void)
{
    uint8_t packet[] = {2, Command_ID_USB_Device_Ping};

    if (HERMES_VERBOSITY_USB > 1)
    {
        printf("LOG 2: Adding ping to stack\n");
    }

    return Hermes_Add_Command_To_Stack(packet, sizeof(packet));
}

int hermes_add_delay_ms(int delay)
{

    uint8_t packet[] = {
        6,                              // Total packet length
        Command_ID_USB_Device_Delay_Ms, // Command ID
        (uint8_t)(delay & 0xFF),        // LSB
        (uint8_t)((delay >> 8) & 0xFF),
        (uint8_t)((delay >> 16) & 0xFF),
        (uint8_t)((delay >> 24) & 0xFF) // MSB
    };

    if (HERMES_VERBOSITY_USB > 1)
    {
        printf("LOG 2: Adding delay to stack: %d ms\n", delay);
    }

    return Hermes_Add_Command_To_Stack(packet, sizeof(packet));
}

int hermes_add_I2C_read(uint8_t addr, uint8_t len)
{

    uint8_t packet[] = {
        4,                              // Total packet length
        Command_ID_USB_Device_I2C_Read, // Command ID
        addr,                           // addr
        len,                            // len
    };

    if (HERMES_VERBOSITY_USB > 1)
    {
        printf("LOG 2: Adding I2C Read to stack\n");
    }

    return Hermes_Add_Command_To_Stack(packet, sizeof(packet));
}