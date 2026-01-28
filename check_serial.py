import serial
import time

port = 'COM5'
baud = 115200

print(f"Listening on {port} at {baud} baud...")
print("Press Ctrl+C to stop\n")

ser = serial.Serial(port, baud, timeout=1)
time.sleep(1)

# Clear buffer
ser.reset_input_buffer()

print("Waiting for any serial output from device...")
print("-" * 60)

try:
    while True:
        if ser.in_waiting > 0:
            data = ser.read(ser.in_waiting)
            print(data.decode('utf-8', errors='ignore'), end='')
        time.sleep(0.1)
except KeyboardInterrupt:
    print("\n\nStopped.")
    ser.close()
