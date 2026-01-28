#!/usr/bin/env python3
"""
Test bidirectional CAN communication
Send a J1939 request and listen for responses
"""

import requests
import time

BASE_URL = "http://192.168.7.116"

print("="*70)
print("BIDIRECTIONAL CAN TEST")
print("="*70)

# Check initial status
print("\n1. Initial CAN Status:")
status = requests.get(f"{BASE_URL}/api/can/status").json()
print(f"   Ready: {status['ready']}")
print(f"   State: {status['state']} (1=RUNNING)")
print(f"   TX Errors: {status['tx_errors']}")
print(f"   RX Errors: {status['rx_errors']}")

# Send a J1939 request message (PGN 59904 - Request)
print("\n2. Sending J1939 Request Message (PGN EA00)...")
payload = {
    "pgn": "EA00",  # Request PGN
    "priority": 6,
    "source_address": "F9",
    "destination_address": "FF",  # Broadcast
    "data": "00 EE 00"  # Request PGN EE00 (Engine Hours)
}

response = requests.post(f"{BASE_URL}/api/can/send", json=payload)
print(f"   Send result: {response.json()}")

# Listen for responses for 5 seconds
print("\n3. Listening for responses (5 seconds)...")
messages_received = 0
start_time = time.time()

while time.time() - start_time < 5:
    response = requests.get(f"{BASE_URL}/api/can/receive?timeout=1000", timeout=2)
    if response.status_code == 200:
        data = response.json()
        if data.get('success') and data.get('messages'):
            for msg in data['messages']:
                messages_received += 1
                print(f"   ✓ Received: ID=0x{msg['id']:X}, Data={msg['data']}")

# Final status
print("\n4. Final CAN Status:")
status = requests.get(f"{BASE_URL}/api/can/status").json()
print(f"   TX Errors: {status['tx_errors']}")
print(f"   RX Errors: {status['rx_errors']}")
print(f"   Messages Received: {messages_received}")

print("\n" + "="*70)
if messages_received > 0:
    print("✓ CAN RX WORKING - Received responses!")
else:
    print("✗ CAN RX NOT WORKING - No responses received")
    print("\nPossible causes:")
    print("  - POWERCELL devices not responding to requests")
    print("  - POWERCELL devices not powered/active")
    print("  - CAN transceiver RX not connected to ESP32 GPIO 19")
    print("  - TJA1051 transceiver not powered (check 3.3V/5V)")
print("="*70)
