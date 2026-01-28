import serial
import time
import sys

port = 'COM5'
baud = 115200

print(f"Attempting to connect to {port}...")

# Try to connect
try:
    ser = serial.Serial(port, baud, timeout=2, exclusive=True)
    print(f"✓ Connected to {port} at {baud} baud\n")
    time.sleep(2)
    
    # Clear any stale data
    ser.reset_input_buffer()
    ser.reset_output_buffer()
    
    print("Sending newline to trigger prompt...")
    ser.write(b"\n")
    time.sleep(0.5)
    
    # Read any boot messages or responses
    if ser.in_waiting > 0:
        print(f"Received data ({ser.in_waiting} bytes):")
        data = ser.read(ser.in_waiting)
        print(data.decode('utf-8', errors='ignore'))
    else:
        print("No data received yet.\n")
    
    print("Sending 'help' command...")
    ser.write(b"help\r\n")
    time.sleep(1)
    
    if ser.in_waiting > 0:
        print(f"Response ({ser.in_waiting} bytes):")
        data = ser.read(ser.in_waiting)
        print(data.decode('utf-8', errors='ignore'))
    else:
        print("No response to 'help' command")
        
    print("\nSending 'canstatus' command...")
    ser.write(b"canstatus\r\n")
    time.sleep(1)
    
    if ser.in_waiting > 0:
        print(f"Response ({ser.in_waiting} bytes):")
        data = ser.read(ser.in_waiting)
        print(data.decode('utf-8', errors='ignore'))
    else:
        print("No response to 'canstatus' command")
    
    ser.close()
    print("\nConnection closed.")
    
except serial.SerialException as e:
    print(f"✗ Error: {e}")
    print("\nThe COM port might be in use by another program.")
    print("Close PlatformIO Serial Monitor or other serial terminals and try again.")
    sys.exit(1)
