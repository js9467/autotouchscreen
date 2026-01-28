#!/usr/bin/env python3
import requests
import time
import json

print("Sending CAN test frame to InfinityBox...")
print("PGN: 0xFF41 (poll command)")
print("Data: 11 00 00 00 00 00 00 00\n")

try:
    # Send test frame
    payload = {
        "pgn": "FF41",
        "data": [17, 0, 0, 0, 0, 0, 0, 0]
    }
    
    response = requests.post(
        "http://192.168.7.116/api/can/send",
        json=payload,
        timeout=5
    )
    
    print(f"TX Response: {response.status_code}")
    print(f"  {json.dumps(response.json(), indent=2)}\n")
    
    # Wait and check for responses
    print("Listening for CAN responses (2 seconds)...")
    time.sleep(2)
    
    response = requests.get(
        "http://192.168.7.116/api/can/receive?timeout=2000",
        timeout=5
    )
    
    print(f"RX Response: {response.status_code}")
    data = response.json()
    print(f"  Received {data['count']} frames")
    
    if data['count'] > 0:
        print("\n=== CAN Traffic ===")
        for msg in data['messages']:
            data_hex = ' '.join([f"{b:02X}" for b in msg['data']])
            print(f"  ID: 0x{msg['id']} Data: {data_hex}")
    else:
        print("  (No CAN frames received)")
        
except Exception as e:
    print(f"ERROR: {e}")
