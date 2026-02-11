#pragma once

// I2C Slave Commands
#define Command_ID_I2C_Slave_Reset_MCU 0x00 // (universal)

#define Command_ID_I2C_Slave_Jump_To_Bootloader 0x11	// (bootloader)
#define Command_ID_I2C_Slave_Flash_Set_Pointer 0x12		// (bootloader)
#define Command_ID_I2C_Slave_Flash_Read_Page 0x13		// (bootloader)
#define Command_ID_I2C_Slave_Flash_Write_Page 0x14		// (bootloader)
#define Command_ID_I2C_Slave_Flash_Check_For_Error 0x15 // (bootloader)
#define Command_ID_I2C_Slave_Flash_Get_Version 0x16

#define Command_ID_I2C_Slave_GPIO_Set 0x20 // (dark_times)

// USB Device Commands
#define Command_ID_USB_Device_I2C_Write 0x01
#define Command_ID_USB_Device_I2C_Send_Receive 0x02
#define Command_ID_USB_Device_Echo 0x03
#define Command_ID_USB_Device_Ping 0x04
#define Command_ID_USB_Device_Delay_Ms 0x05
#define Command_ID_USB_Device_I2C_Read 0x06

#define Command_ID_USB_Device_PC_Short_Data_Return 0xFF
