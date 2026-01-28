import serial
import time
import sys

PORT = 'COM5'
BAUD = 115200

print(f"Connecting to {PORT} at {BAUD} baud...")
try:
    ser = serial.Serial(PORT, BAUD, timeout=2)
except serial.SerialException as e:
    print(f"Error opening {PORT}: {e}")
    sys.exit(1)

# Nudge reset via DTR/RTS (helps some boards)
try:
    ser.setDTR(False)
    ser.setRTS(False)
    time.sleep(0.1)
    ser.setDTR(True)
    ser.setRTS(True)
except Exception:
    pass

time.sleep(0.5)

# Clear buffers
ser.reset_input_buffer()
ser.reset_output_buffer()

# Helper to send and read

def send_and_read(cmd, wait=1.5):
    print(f"\n>>> {cmd}")
    ser.write((cmd + "\r\n").encode())
    time.sleep(wait)
    resp = b''
    for _ in range(20):
        if ser.in_waiting:
            resp += ser.read(ser.in_waiting)
        time.sleep(0.05)
    text = resp.decode('utf-8', errors='ignore')
    print(text if text else "<no response>")

# Try basic commands
send_and_read('help', 2.0)
send_and_read('canstatus', 2.0)

# Try enabling/disabling OTA to reduce noise
send_and_read('otaoff', 1.0)
send_and_read('canstatus', 1.5)

# Attempt raw CAN send and brief monitor
send_and_read('cansend FF41 11 00 00 00 00 00 00 00', 1.5)
send_and_read('canmon', 11.0)

ser.close()
print("\nDone.")
