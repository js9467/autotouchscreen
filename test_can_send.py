#!/usr/bin/env python3
"""Send test CAN frames to InfinityBox and monitor responses"""
import requests
import json
import time
from datetime import datetime

DEVICE = "http://192.168.7.116"

def send_frame(pgn, data, priority=6, source=0x00, destination=0xFF):
    """Send a CAN frame via web API"""
    payload = {
        "pgn": pgn,
        "priority": priority,
        "source": source,
        "destination": destination,
        "data": data
    }
    
    try:
        response = requests.post(
            f"{DEVICE}/api/can/send",
            json=payload,
            timeout=5
        )
        
        status = "✓" if response.status_code == 200 else "✗"
        data_hex = ' '.join([f"{b:02X}" for b in data])
        print(f"{status} Sent PGN 0x{pgn:04X} [{data_hex}]")
        return response.status_code == 200
        
    except Exception as e:
        print(f"✗ Failed to send: {e}")
        return False

print("=" * 60)
print("  CAN FRAME SENDER - Testing InfinityBox Communication")
print("=" * 60)
print(f"Start time: {datetime.now().strftime('%H:%M:%S')}\n")

# Test various common J1939 PGNs
print("Step 1: Sending power status poll to InfinityBox...")
send_frame(0xFF41, [0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00])
time.sleep(1)

print("\nStep 2: Sending request for device info...")
send_frame(0xEA00, [0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00])
time.sleep(1)

print("\nStep 3: Sending generic control frame...")
send_frame(0x0000, [0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00])
time.sleep(1)

print("\nStep 4: Listening for 5 seconds for any responses...\n")
for i in range(5):
    try:
        response = requests.get(
            f"{DEVICE}/api/can/receive?timeout=1000",
            timeout=5
        )
        
        data = response.json()
        if data['count'] > 0:
            print(f"[{datetime.now().strftime('%H:%M:%S')}] Received {data['count']} frame(s):")
            for msg in data['messages']:
                data_hex = ' '.join([f"{b:02X}" for b in msg['data']])
                print(f"  ID: 0x{msg['id']:08s}  Data: {data_hex}")
        else:
            print(f"  No frames yet...")
            
    except Exception as e:
        print(f"  Listen error: {e}")
    
    time.sleep(1)

print("\nTest complete!")
