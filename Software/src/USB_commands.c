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