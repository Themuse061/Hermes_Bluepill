#ifndef COMMANDS_MK
#define COMMANDS_MK

// I2C Slave Commands
#define I2C_Slave_Command_Reset_MCU                  0x00 // (universal)
#define I2C_Slave_Command_Jump_To_Bootloader         0x01 // (universal)
#define I2C_Slave_Command_Flash_Set_Pointer          0x02 // (universal)
#define I2C_Slave_Command_Flash_Read_Page            0x03 // (universal)
#define I2C_Slave_Command_Flash_Write_Page           0x04 // (universal)
#define I2C_Slave_Command_Flash_Check_For_Error      0x05 // (universal)

#define I2C_Slave_Command_GPIO_Set              0x10 // (dark_times)

// USB Device Commands
#define USB_Device_Command_I2C_Write            0x01
#define USB_Device_Command_I2C_Send_Receive     0x02
#define USB_Device_Command_Echo                 0x03
#define USB_Device_Command_Ping                 0x04
#define USB_Device_Command_Delay_Ms          0x05

#define USB_Device_Command_PC_Short_Data_Return 0xFF

#endif // COMMANDS_MK

 Command description
  I2C
  0x10 - GPIO Set
    Master sends: [Addr] [CMD_GPIO] [Bitmask]
    Updates GPIO pins based on bitmask:
    - Bit 0: PA2
    - Bit 1: PD6
    - Bit 2: PC4

  USB
  0x01 - I2C Write
    Writes data to I2C1 bus.
    Format: [Packet Length] [0x01] [Target Address] [Data...]

  0x02 - I2C Send/Receive
    Writes data to I2C bus, then reads data back.
    Format: [Packet Length] [0x02] [Target Address] [Write Length] [Read Length] [Write Data...]
    Returns: [Packet Length] [0xFF] [0x02] [Address] [Read Data...]

  0x03 - Echo
    Echoes the received packet back to the host.
    Format: [Packet Length] [0x03] [Data...]

  0x04 - Ping
    Returns a fixed ping response.
    Format: [Packet Length] [0x04] [Data...]
    Response: 09 04 FF aa 00 11 00 aa FF



  0xFF - PC Short Data Return
    Packet sent from MCU to PC containing requested data (e.g., from I2C Read).
    Format: [Packet Length] [0xFF] [Source Command] [Address] [Data...]
