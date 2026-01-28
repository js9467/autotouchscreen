#!/usr/bin/env python3
"""Query device to check CAN pin configuration and status"""
import requests
import json

DEVICE = "http://192.168.7.116"

print("=" * 60)
print("  CAN PIN CONFIGURATION CHECK")
print("=" * 60)

# Check device info
try:
    response = requests.get(f"{DEVICE}/api/status")
    status = response.json()
    print(f"\nDevice Status:")
    print(f"  Firmware: {status.get('firmware_version')}")
    print(f"  IP: {status.get('sta_ip')}")
    print(f"  Free RAM: {status.get('free_heap')}")
except Exception as e:
    print(f"Error getting status: {e}")
    exit(1)

# Try to check serial console output for CAN init messages
print(f"\nExpected CAN Configuration (from code):")
print(f"  TX Pin: GPIO 19")
print(f"  RX Pin: GPIO 20")
print(f"  Bitrate: 250 kbps")
print(f"  Mode: NO_ACK (passive listening)")
print(f"  USB_SEL: HIGH (via IO Expander pin 5)")

# Test sending a frame to check if TWAI is initialized
print(f"\n[TEST] Sending CAN frame to verify TWAI is running...")
try:
    payload = {
        "pgn": 0x0001,
        "priority": 6,
        "source": 0x00,
        "destination": 0xFF,
        "data": [0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x11]
    }
    
    response = requests.post(f"{DEVICE}/api/can/send", json=payload, timeout=5)
    if response.status_code == 200:
        print(f"  ✓ TWAI driver appears to be initialized (send succeeded)")
    else:
        print(f"  ✗ Send failed with status {response.status_code}")
        print(f"    Response: {response.text}")
        
except Exception as e:
    print(f"  ✗ Error: {e}")

print(f"\n" + "=" * 60)
print(f"POSSIBLE SOLUTIONS IF NO CAN TRAFFIC:")
print(f"  1. Try different GPIO pins (16,17,18,21,22,etc)")
print(f"  2. Verify USB_SEL high signal is actually reaching expander")
print(f"  3. Check if CAN transceiver is connected/powered")
print(f"  4. Verify CAN H/L wires connected to InfinityBox")
print(f"=" * 60)
