
#define USB_Command_Byte_Length 0
#define USB_Command_Byte_Command 1
#define USB_Command_Byte_Data 2
#define USB_Command_data_size 64
#define USB_Command_max_length USB_Command_data_size + 2

// Usb command handlers
#define Command_ID_USB_Device_I2C_Write 0x01
void USB_command_handler_I2C_write(uint8_t *command_array);

#define Command_ID_USB_Device_I2C_Send_Receive 0x02
void USB_command_handler_I2C_send_recieve(uint8_t *command_array);

#define Command_ID_USB_Device_Echo 0x03
void USB_command_handler_echo(uint8_t *command_array);

#define Command_ID_USB_Device_Ping 0x04
void USB_command_handler_ping(uint8_t *command_array);

#define Command_ID_USB_Device_Delay_Ms 0x05
void USB_command_handler_delay_ms(uint8_t *command_array);

#define Command_ID_USB_Device_PC_Short_Data_Return 0xFF

/*
USB Packet format

1. Length (including Length byte)
2. Command type (0x00 - 0xFF)
3. Data (up to USB_Command_data_size)
*/