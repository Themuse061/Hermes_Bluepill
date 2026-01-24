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
        printf("VERBOSE: Adding ping to stack");
    }

    return Hermes_Add_Command_To_Stack(packet, sizeof(packet));
}
