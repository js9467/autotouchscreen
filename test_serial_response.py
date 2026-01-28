#!/usr/bin/env python3
"""Test serial communication with device"""
import serial
import time

try:
    ser = serial.Serial('COM5', 115200, timeout=2)
    print("✓ Serial port opened")
    
    # Send a newline to see if we get a prompt
    ser.write(b'\n')
    time.sleep(0.5)
    
    # Try to read response
    response = ser.readline().decode('utf-8', errors='ignore')
    print(f"Response: {repr(response)}")
    
    if response:
        print("✓ Serial is responsive!")
        # Try a command
        ser.write(b'help\n')
        time.sleep(0.5)
        data = ser.read(1024).decode('utf-8', errors='ignore')
        print("\nDevice response to 'help':")
        print(data[:500])
    else:
        print("✗ No response from device")
    
    ser.close()
    
except Exception as e:
    print(f"✗ Error: {e}")
