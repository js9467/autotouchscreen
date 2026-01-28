#!/usr/bin/env python3
"""Simple serial monitor to watch device boot and CAN traffic"""
import serial
import time
import sys

PORT = 'COM5'
BAUD = 115200

print(f"Opening {PORT} at {BAUD}...")
try:
    ser = serial.Serial(PORT, BAUD, timeout=0.1)
except Exception as e:
    print(f"Error: {e}")
    sys.exit(1)

print("Connected. Watching for data (Ctrl+C to exit)...\n")

try:
    while True:
        if ser.in_waiting:
            data = ser.read(ser.in_waiting)
            try:
                print(data.decode('utf-8', errors='ignore'), end='', flush=True)
            except:
                print(f"[binary: {data.hex()}]", flush=True)
        time.sleep(0.01)
except KeyboardInterrupt:
    print("\n\nClosing...")
    ser.close()
