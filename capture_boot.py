"""Capture device boot messages after reset"""
import serial
import time

ser = serial.Serial('COM5', 115200, timeout=1)
ser.reset_input_buffer()

print("="*70)
print("PRESS THE RESET BUTTON ON THE DEVICE NOW")
print("="*70)
print("\nCapturing boot messages for 15 seconds...\n")

start = time.time()
while time.time() - start < 15:
    if ser.in_waiting:
        line = ser.readline().decode('utf-8', errors='replace').strip()
        if line:
            print(line)

ser.close()
