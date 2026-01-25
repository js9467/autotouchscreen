#!/usr/bin/env python3
"""
Simple serial monitor to see what the device is outputting
"""

import serial
import time

try:
    print("Connecting to COM5...")
    ser = serial.Serial('COM5', 115200, timeout=1)
    time.sleep(2)
    
    print("=" * 60)
    print("Serial Monitor - COM5 @ 115200 baud")
    print("Press Ctrl+C to exit")
    print("=" * 60)
    print()
    
    # Try resetting device with DTR/RTS
    print("Attempting to reset device...")
    ser.setDTR(False)
    ser.setRTS(False)
    time.sleep(0.1)
    ser.setDTR(True)
    ser.setRTS(True)
    time.sleep(2)
    
    while True:
        if ser.in_waiting > 0:
            data = ser.read(ser.in_waiting)
            try:
                print(data.decode('utf-8', errors='replace'), end='', flush=True)
            except:
                print(f"[Binary: {data.hex()}]", end='', flush=True)
        time.sleep(0.01)
        
except KeyboardInterrupt:
    print("\n\nExiting...")
except Exception as e:
    print(f"Error: {e}")
finally:
    if 'ser' in locals():
        ser.close()
    print("Serial connection closed")
