#!/usr/bin/env python3
"""Test serial connection to device"""

import serial
import time
import sys

try:
    ser = serial.Serial('COM5', 115200, timeout=1)
    print(f'✓ Serial port opened: {ser.name}')
    ser.reset_input_buffer()
    ser.reset_output_buffer()
    
    print('Waiting 2 seconds for device data...')
    time.sleep(2)
    
    # Check for any boot messages
    if ser.in_waiting:
        data = ser.read(ser.in_waiting)
        text = data.decode(errors='ignore')
        print(f'✓ Received data ({len(data)} bytes):')
        print(text[:500])
    else:
        print('❌ No data available - device not responding')
    
    # Try sending a newline
    print('\nSending newline to check responsiveness...')
    ser.write(b'\n')
    time.sleep(0.5)
    
    if ser.in_waiting:
        response = ser.read(ser.in_waiting)
        print(f'Response: {response.decode(errors="ignore")}')
    else:
        print('❌ No response to newline')
    
    ser.close()
    print('\n✓ Serial test complete')
    
except Exception as e:
    print(f'❌ Error: {e}')
    sys.exit(1)
