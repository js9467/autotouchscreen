#!/usr/bin/env python3
"""CAN Bus Diagnostic Tool - Tests transceiver and bus connectivity"""
import requests
import json
import time
from datetime import datetime

DEVICE = "http://192.168.7.116"

print("=" * 70)
print("  CAN BUS DIAGNOSTIC TOOL")
print("=" * 70)

# Step 1: Check device connectivity
print("\n[1] Testing device connectivity...")
try:
    response = requests.get(f"{DEVICE}/api/status", timeout=5)
    status = response.json()
    print(f"  ✓ Device online: {status['sta_ip']}")
    print(f"  ✓ Firmware: {status['firmware_version']}")
except Exception as e:
    print(f"  ✗ Device unreachable: {e}")
    exit(1)

# Step 2: Test if CAN API endpoint exists
print("\n[2] Testing CAN API endpoints...")
try:
    response = requests.get(f"{DEVICE}/api/can/receive?timeout=100", timeout=5)
    if response.status_code == 200:
        data = response.json()
        print(f"  ✓ /api/can/receive endpoint working")
        print(f"    Current frame buffer: {data.get('count', 0)} frames")
    else:
        print(f"  ✗ Endpoint returned {response.status_code}")
except Exception as e:
    print(f"  ✗ Cannot reach CAN endpoint: {e}")

# Step 3: Send a frame and listen for echo (if loopback is possible)
print("\n[3] Sending test frame to measure round-trip...")
try:
    payload = {
        "pgn": 0xDEAD,
        "priority": 6,
        "source": 0xFF,
        "destination": 0xFF,
        "data": [0xBE, 0xEF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00]
    }
    
    response = requests.post(f"{DEVICE}/api/can/send", json=payload, timeout=5)
    if response.status_code == 200:
        print(f"  ✓ Frame sent successfully")
        
        # Listen for 2 seconds to see if we get any response
        print("  Listening for echo or response (2 seconds)...")
        time.sleep(2)
        
        response = requests.get(f"{DEVICE}/api/can/receive?timeout=100", timeout=5)
        data = response.json()
        
        if data['count'] > 0:
            print(f"  ✓ Received {data['count']} frame(s) - CAN BUS IS WORKING!")
            for msg in data['messages']:
                print(f"    ID: 0x{msg['id']}  Data: {msg['data']}")
        else:
            print(f"  ✗ No frames received - possible issue:")
            print(f"    - CAN bus not connected to device")
            print(f"    - Transceiver not powered")
            print(f"    - No other devices on bus")
    else:
        print(f"  ✗ Send failed with status {response.status_code}")
        
except Exception as e:
    print(f"  ✗ Error during test: {e}")

# Step 4: Check what the firmware expects
print("\n[4] Firmware CAN Configuration (from code):")
print("  GPIO TX:   19")
print("  GPIO RX:   20")
print("  Bitrate:   250 kbps")
print("  Mode:      NO_ACK (passive listening)")
print("  Filter:    Accept all frames")

# Step 5: Recommendations
print("\n[5] NEXT STEPS TO VERIFY HARDWARE:")
print("  □ Check physical CAN wires at device:")
print("    - H line (green/yellow) -> connected to InfinityBox CAN H")
print("    - L line (white) -> connected to InfinityBox CAN L")
print("  □ Check for 12V power to CAN transceiver module")
print("  □ If using separate CAN transceiver (not on main board):")
print("    - Verify R/STBY pin is pulled LOW (or floating)")
print("    - Check 120Ω termination resistors on H-L (both ends of bus)")
print("  □ Verify InfinityBox is powered and transmitting")

print("\n" + "=" * 70)
print("  Run this diagnostic again after checking connections")
print("=" * 70)
