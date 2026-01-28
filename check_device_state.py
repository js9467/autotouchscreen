#!/usr/bin/env python3
"""
Check what state the ESP32 device is currently in
"""

import serial
import time

port = "COM5"
baud = 115200

print("Opening port and checking for boot messages...\n")

try:
    ser = serial.Serial(port, baud, timeout=2)
    
    # Clear buffers
    ser.reset_input_buffer()
    ser.reset_output_buffer()
    
    print(f"✓ Port opened: {port}")
    print("Waiting 3 seconds for any boot messages...\n")
    
    time.sleep(3)
    
    if ser.in_waiting:
        data = ser.read(ser.in_waiting)
        print(f"✓ Received {len(data)} bytes:")
        print("=" * 60)
        try:
            text = data.decode('utf-8', errors='replace')
            print(text)
        except:
            print(data[:500])
        print("=" * 60)
        
        # Analyze output
        text_lower = data.decode('utf-8', errors='replace').lower()
        if 'bootloader' in text_lower or 'rom' in text_lower:
            print("\n✓ Device is responding! (bootloader active)")
        elif 'rst:0x' in text_lower or 'wdt' in text_lower:
            print("\n✓ Device is responding! (may be stuck in boot loop)")
        else:
            print("\n✓ Device is responding! (running firmware)")
    else:
        print("✗ NO DATA RECEIVED from device")
        print("\nThis means:")
        print("  • Device is not powered on, OR")
        print("  • Device is frozen/not sending serial data, OR")
        print("  • USB cable doesn't have data lines (power-only cable), OR")
        print("  • Serial driver issue")
        
        # Try to send a carriage return to trigger any response
        print("\nAttempting to wake device with reset signal...")
        ser.dtr = False  # Reset signal
        time.sleep(0.1)
        ser.dtr = True
        time.sleep(1)
        
        if ser.in_waiting:
            data = ser.read(ser.in_waiting)
            print(f"\n✓ Device responded to reset! Received {len(data)} bytes:")
            print(data.decode('utf-8', errors='replace')[:500])
        else:
            print("✗ Device did not respond to reset signal")
    
    ser.close()
    
except serial.SerialException as e:
    print(f"✗ Cannot open port: {e}")
except Exception as e:
    print(f"✗ Error: {e}")
