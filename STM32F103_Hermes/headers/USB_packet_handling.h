
#define USB_Command_Byte_Length 0
#define USB_Command_Byte_Command 1
#define USB_Command_Byte_Data 2
#define USB_Command_data_size 64
#define USB_Command_max_length USB_Command_data_size + 2

// Usb command handlers
#define USB_command_handler_I2C_write_number 0x01
void USB_command_handler_I2C_write(uint8_t *command_array);

#define USB_command_handler_I2C_send_recieve_number 0x02
void USB_command_handler_I2C_send_recieve(uint8_t *command_array);

#define USB_command_PC_short_data_return 0xFF

/*
USB Packet format

1. Length (including Length byte)
2. Command type (0x00 - 0xFF)
3. Data (up to USB_Command_data_size)
*/