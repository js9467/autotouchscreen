#!/usr/bin/env python3
"""
Test multiple GPIO pin combinations for CAN activity
This script will send frames and listen on both GPIO configurations
"""
import requests
import json
import time
from datetime import datetime

DEVICE = "http://192.168.7.116"

print("=" * 70)
print("  TESTING GPIO CONFIGURATIONS FOR CAN BUS")
print("=" * 70)

# The device can only use one pin set at a time, so this is a theoretical test
# But we can infer what the correct pins are based on success/failure

PIN_CONFIGS = [
    {"name": "GPIO 19/20 (Current)", "tx": 19, "rx": 20},
    {"name": "GPIO 16/17 (Alt 1)", "tx": 16, "rx": 17},
    {"name": "GPIO  4/5  (Alt 2)", "tx": 4, "rx": 5},
    {"name": "GPIO  9/10 (Alt 3)", "tx": 9, "rx": 10},
]

print("\nCurrent Pin Configuration (from device firmware):")
print("  GPIO 19/20")

print("\nDIAGNOSTIC TEST:")
print(f"[1] Sending test frame...")

try:
    payload = {
        "pgn": 0x0001,
        "priority": 6,
        "source": 0xAA,
        "destination": 0xBB,
        "data": [0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88]
    }
    
    response = requests.post(f"{DEVICE}/api/can/send", json=payload, timeout=5)
    if response.status_code == 200:
        print(f"  ✓ Frame sent")
    else:
        print(f"  ✗ Send failed: {response.status_code}")

except Exception as e:
    print(f"  ✗ Error: {e}")
    exit(1)

print(f"\n[2] Listening for received frames (5 seconds)...")

frame_count = 0
frames_received = []

for i in range(5):
    try:
        response = requests.get(f"{DEVICE}/api/can/receive?timeout=1000", timeout=5)
        data = response.json()
        
        if data['count'] > 0:
            frame_count += data['count']
            frames_received.extend(data['messages'])
            print(f"  Frame {i+1}: Received {data['count']} message(s)")
            
    except Exception as e:
        pass

print(f"\nRESULTS:")
print(f"  Total frames received: {frame_count}")

if frame_count > 0:
    print(f"\n  ✓ SUCCESS! CAN bus is receiving traffic!")
    print(f"\n  Frames received:")
    for msg in frames_received[:5]:  # Show first 5
        data_hex = ' '.join([f"{b:02X}" for b in msg['data']])
        print(f"    ID: 0x{msg['id']:08s}  Data: {data_hex}")
    
    if len(frames_received) > 5:
        print(f"    ... and {len(frames_received)-5} more")
else:
    print(f"\n  ✗ FAILURE - No CAN frames received")
    print(f"\nPOSSIBLE CAUSES:")
    print(f"  1. GPIO pins wrong (currently using 19/20)")
    print(f"     Try building with GPIO 16/17 or 4/5")
    print(f"  2. CAN transceiver not powered")
    print(f"  3. CAN bus not connected properly")
    print(f"  4. USB_SEL signal not reaching the IO expander")

print("\n" + "=" * 70)
