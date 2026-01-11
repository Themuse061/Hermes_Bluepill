import serial
import threading
import sys
import time

# --- CONFIGURATION ---
COM_PORT = 'COM14'  # Change this to your port (e.g., 'COM3' on Windows, '/dev/ttyUSB0' on Linux)
BAUD_RATE = 115200   # Match the baud rate of your device
def read_from_port(ser):
    """
    Background thread function that reads from the serial port
    and prints data to the console immediately.
    """
    while True:
        try:
            # Check if there is data waiting in the buffer
            if ser.in_waiting > 0:
                # Read the bytes and decode them to a string
                data = ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
                # Print without adding an extra newline (the device usually sends its own)
                sys.stdout.write(data)
                sys.stdout.flush()
            time.sleep(0.01)  # tiny sleep to reduce CPU usage
        except serial.SerialException:
            print("\nError reading from port. Connection lost.")
            break

def main():
    try:
        # Open the serial port
        ser = serial.Serial(COM_PORT, BAUD_RATE, timeout=1)
        print(f"Connected to {COM_PORT} at {BAUD_RATE} baud.")
        print("Type your message and press Enter to send.")
        print("Press Ctrl+C to exit.\n")
    except Exception as e:
        print(f"Failed to open port {COM_PORT}. Error: {e}")
        return

    # Start the background thread for receiving data
    read_thread = threading.Thread(target=read_from_port, args=(ser,))
    read_thread.daemon = True # Thread dies when main program exits
    read_thread.start()

    # Main loop for sending keyboard input
    try:
        while True:
            # Read input from the user (blocks until Enter is pressed)
            user_input = input()

            # Send the data. Note: We usually add a newline character (\n)
            # because 'input()' strips it, and serial devices usually expect it.
            if ser.is_open:
                ser.write((user_input + '\n').encode('utf-8'))

    except KeyboardInterrupt:
        print("\nExiting program...")
    finally:
        ser.close()
        print("Port closed.")

if __name__ == "__main__":
    main()