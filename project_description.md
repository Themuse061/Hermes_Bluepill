# Project Architecture: Hermes Bluepill System

## 1. Project Overview
The **Hermes Bluepill** system is a hardware-software bridge designed to interface a PC with a CH32V003 microcontroller via a USB-to-I2C bridge (assumed to be the "Hermes" device, likely STM32F103-based).

**Key Components:**
*   **Host PC Software:** A C-based test suite and driver layer that manages USB communication, command batching, and high-level protocols.
*   **Bridge (Hermes):** (Implied) A USB device that receives batched commands from the PC and executes them as I2C transactions.
*   **Target Firmware (CH32V003):** A specialized bootloader/application running on the CH32V003 RISC-V MCU, acting as an I2C slave.

**Primary Goal:**
To provide a robust, latency-tolerant mechanism for controlling the CH32V003, reading its flash memory, and potentially managing bootloader operations (reset, jump) from a PC.

---

## 2. System Architecture

The system follows a tiered architecture designed to minimize USB transaction overhead by batching low-level I2C operations.

**High-Level Flow:**
```text
[PC App (Test Suite)]
       |
       v
[PC I2C Abstraction Layer] (Batches commands into a Stack)
       |
       v
[USB Transport Layer] (Sends Stack to Bridge)
       |
       v
[USB Connection]
       |
       v
[Hermes Bridge] (Parses Stack, Executes I2C)
       |
       v
[I2C Bus]
       |
       v
[CH32V003 Target] (I2C Slave, Register Map)
```

**Responsibilities:**
*   **PC:** Orchestration, logic, protocol encapsulation, and timing control (via batched delay commands).
*   **Firmware:** Passive execution of I2C commands, exposing internal state (Flash pointer, Reset trigger) via a virtual register map.

---

## 3. Firmware (CH32V003 Bootloader)

### Architecture
The firmware implements an **interrupt-driven I2C Slave** model using a virtual register map approach. It does not process a command stream in the main loop; instead, it reacts immediately to I2C events via interrupts.

### Boot Process & Execution
*   **Entry:** Standard startup `SystemInit`, Clock Enable, GPIO Config.
*   **Main Loop:** Purely cosmetic/status-oriented. It updates an LED pattern based on the `last_command_received` variable (Idle, Got Pointer, or Reading).
*   **Logic Core:** All functional logic is inside `I2C1_EV_IRQHandler` (via `onWrite` and `onRead` callbacks).

### Memory & Flash Strategy
*   **Flash Pointer:** Maintained in volatile memory (`flash_pointer`), defaulting to `0x08000000`.
*   **Read Access:** Allows reading flash memory from `0x08000000` to `0x08004000`.
*   **Write Access:** **Not Detected.** The analyzed `bootloader.c` source does not explicitly implement `Command_ID_I2C_Slave_Flash_Write_Page`. The firmware appears to be a read-only inspector or a simplified test-stub in its current state.
*   **Linker:** Uses a specific linker script (`generated_ch32v003_For_bootloader.ld`) constraining the binary to 2KB (`max_size = 2048`).

### Command Handling (Register Map)
The I2C slave treats the first written byte as a "Register Address" (Command ID).
*   **Reset (0x00):** Triggers `raw_reset()`.
*   **Set Flash Pointer (0x02):** Expects 2 bytes (LSB, MSB) following the command. Updates `flash_pointer`.
*   **Read Flash Page (0x03):** Pre-loads the buffer with the byte at `flash_pointer` for subsequent reading.

#### Code Example: Firmware Command Parsing (`bootloader.c`)
```c
// Inside onWrite callback
switch (reg)
{
case Command_ID_I2C_Slave_Reset_MCU:
    raw_reset();
    break;

case Command_ID_I2C_Slave_Flash_Set_Pointer:
{
    // Reconstructs 16-bit offset from next two bytes in buffer
    uint16_t offset = i2c_buffer[reg] | (i2c_buffer[reg + 1] << 8);
    flash_pointer = 0x08000000 + offset;
}
break;
// ...
}
```

---

## 4. PC Software

### Overall Architecture
The PC software is built as a command-line tool relying on a custom "Hermes" USB protocol. It emphasizes **deferred execution**: commands are not sent immediately but are pushed onto a "Stack" and flushed in bulk.

### Communication Workflow
1.  **Preparation:** Application calls `Stack_add_...` functions.
2.  **Buffering:** Commands are serialized into a linear byte buffer (`hermes_packet_stack`).
3.  **Transmission:** `Hermes_Flush_Stack()` sends the entire buffer via `libserialport`.
4.  **Reception:** If data return is expected, the software blocks waiting for a USB response.

### Timing Assumptions
*   **Delay Offloading:** Delays are often handled by the Bridge, not the PC. The PC sends a "Delay" command (e.g., 250ms), and the Bridge pauses execution before processing the next command in the stack.
*   **Host Delays:** High-level logic (like `Hera_I2C_jump_to_bootloader`) employs PC-side `Sleep()` calls after flushing to allow the target time to reset or boot.

---

## 5. PC I2C Abstraction Layer (EXTENSIVE)

### Design Goals
The primary goal is **Latency Hiding**. USB round-trip times (1ms+) are significant compared to I2C speeds. Sending individual I2C start/write/stop commands for every byte would be prohibitively slow. The abstraction layer groups sequences (e.g., "Write Addr, Write Data, Wait, Read Data") into a single USB packet.

### API & Abstraction Boundaries
The API resides in `USB_commands.c` and `Hera_Functions.c`.
*   **`Stack_add_I2C_Write(addr, data, len)`:** Encapsulates an I2C Write transaction.
*   **`Stack_add_I2C_Send_recieve(addr, write_len, read_len, write_data)`:** Encapsulates a Write-then-Read transaction (Atomic Start-Write-Restart-Read-Stop implied).
*   **`Stack_add_delay(ms)`:** Adds a pause to the execution queue on the Bridge.

#### Code Example: Adding an I2C Write to the Stack (`USB_commands.c`)
This function constructs the header and queues it without sending USB data immediately.
```c
int Stack_add_I2C_Write(uint8_t I2C_address, uint8_t *data, uint8_t len)
{
    // Packet Header: [Length + 3] [Command ID] [I2C Address]
    uint8_t I2C_header[] = {len + 3, Command_ID_USB_Device_I2C_Write, I2C_address};
    
    // Add Header
    Hermes_Add_Command_To_Stack_Withouta_Advancing_The_Stack_Height(I2C_header, 3);

    // Add Payload (The actual data to write via I2C)
    return Hermes_Add_Command_To_Stack(data, len);
}
```

### Data Flow & Message Structure
**Concept:** The "Stack" is a serialized stream of commands.
*   **PC Internal:** Buffer `hermes_packet_stack` (max 16 commands or buffer limit).
*   **Packet Structure (Conceptual):**
    ```
    [Total Len] [Command ID] [Payload...]
    ```
    *   *Example (I2C Write):* `[Len+3] [0x01] [I2C Addr] [Data...]`

### Error Handling & Retries
*   **Buffer Safety:** Checks for Stack Height (max 16) and Buffer Overflow before adding. Returns error codes (-1 to -4).
*   **Transmission:** `Hermes_Flush_Stack` returns the number of bytes written to USB.
*   **I2C Errors:** **Opaque.** The current abstraction does not appear to proactively report I2C NACKs to the PC application logic in a structured way during the "Stack Add" phase. Errors are likely detected only if the "Read" operation returns unexpected data or if the lower-level driver times out.

### Limitations
*   **Blind Execution:** "Write" commands are fire-and-forget until flushed.
*   **Flow Control:** Relies on the Bridge having sufficient buffer space to receive the entire stack.

---

## 6. Communication and Data Flow

### End-to-End Transaction (Example: Read Flash)
1.  **PC:** `Stack_add_I2C_Send_recieve(Addr, 1, CHUNK_SIZE, Cmd_Read_Page)`
2.  **PC:** `Hermes_Flush_Stack_with_Read(...)`
3.  **USB:** Transmits packet `[Len] [0x02 (Send/Recv)] [Addr] [1] [CHUNK] [0x03]`
4.  **Hermes (Bridge):**
    *   Parses Command.
    *   Sends I2C Start + Addr(Write).
    *   Sends `0x03` (Command ID).
    *   Sends I2C Restart + Addr(Read).
    *   Reads `CHUNK_SIZE` bytes from Target.
    *   Sends I2C Stop.
    *   Buffering: Stores read data.
    *   Returns data via USB to PC.
5.  **Target (CH32V003):**
    *   `onWrite`: Sets buffer index.
    *   `onRead`: Returns flash bytes, increments pointer.
6.  **PC:** Receives data, function returns.

---

## 7. Timing Model

*   **Boot Timing:**
    *   Reset Pulse: 2000ms delay (PC side) allowed after Reset command.
    *   Jump Sequence: Reset -> Wait 250ms (Bridge side) -> Jump Command -> Wait 2000ms (PC side).
*   **Communication Timing:**
    *   I2C Clock: Configured for Fast Mode (33% duty cycle), likely ~400kHz or 1MHz logic clock dependent.
    *   USB Timeout: `HERMES_MAX_TIMEOUT` is 5000ms, indicating tolerance for very long blocking operations (e.g., long flash erase/write cycles if they were implemented).

---

## 8. Key Concepts for AI Context

*   **"The Stack":** The central mechanism for batching commands on the PC before sending to the Bridge.
*   **Virtual Register Map:** The firmware exposes its functions (Reset, Jump, Flash Access) as pseudo-memory addresses accessible via I2C writes.
*   **Hermes:** The intermediate bridge device (STM32F103) that translates USB "Stack" packets into physical I2C signals.
*   **Read-Only Bootloader:** The current codebase analyzed supports reading flash and resetting, but lacks explicit flash writing logic in the main source file.