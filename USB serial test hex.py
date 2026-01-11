import serial
import threading
import sys
import time

# --- CONFIGURATION ---
COM_PORT = 'COM14'   # Change this to your port
BAUD_RATE = 115200   # Match the baud rate of your device

def read_from_port(ser):
    """
    Background thread function that reads from the serial port
    and prints data to the console immediately.
    """
    while True:
        try:
            if ser.in_waiting > 0:
                # Read bytes
                data = ser.read(ser.in_waiting)

                # Attempt to decode as UTF-8 for display, replace errors with ?
                # If you want to see incoming data as HEX, change this line.
                text_data = data.decode('utf-8', errors='replace')

                sys.stdout.write(text_data)
                sys.stdout.flush()
            time.sleep(0.01)
        except serial.SerialException:
            print("\nError reading from port. Connection lost.")
            break
        except OSError:
            break

def main():
    ser = None
    try:
        # Open the serial port
        ser = serial.Serial(COM_PORT, BAUD_RATE, timeout=1)
        print(f"Connected to {COM_PORT} at {BAUD_RATE} baud.")
        print("Type HEX data (e.g., '595A' or '59 5A') and press Enter to send.")
        print("Press Ctrl+C to exit.\n")
    except Exception as e:
        print(f"Failed to open port {COM_PORT}. Error: {e}")
        return

    # Start the background thread for receiving data
    read_thread = threading.Thread(target=read_from_port, args=(ser,))
    read_thread.daemon = True
    read_thread.start()

    # Main loop for sending keyboard input
    try:
        while True:
            # Read input from the user
            user_input = input()

            if ser.is_open and user_input.strip():
                try:
                    # bytes.fromhex ignores whitespace, so "59 5A" and "595A" both work
                    # It converts the hex string directly to raw bytes
                    data_to_send = bytes.fromhex(user_input)

                    # Send raw bytes (no \n added)
                    ser.write(data_to_send)

                    # Optional: Print what was sent for confirmation (comment out if not needed)
                    # print(f" -> Sent {len(data_to_send)} bytes: {data_to_send}")

                except ValueError:
                    print("Error: Input contains non-hex characters or odd length.")

    except KeyboardInterrupt:
        print("\nExiting program...")
    finally:
        if ser and ser.is_open:
            ser.close()
        print("Port closed.")

if __name__ == "__main__":
    main()