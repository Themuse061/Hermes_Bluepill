I2C commands
0x00 - Reset MCU 			(universal)
0x01 - Jump to bootloader	(universal)
0x02 - Write Flash			(universal)
0x03 - Read Flash			(universal)
0x04 - Go to flash address	(universal)

0x10 - GPIO Set 			(dark_times)

USB commands
0x01 - I2C Write
0x02 - I2C Send/Receive
0x03 - Echo
0x04 - Ping
0x05 - delay_ms				(NOT IMPLEMENTED)

0xFF - PC Short Data Return

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
