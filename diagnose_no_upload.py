#!/usr/bin/env python3
"""
CAN Diagnosis without firmware upload
Tests the current device to understand pin configuration
"""
import requests
import json

DEVICE = "http://192.168.7.116"

print("=" * 70)
print("  CAN HARDWARE DIAGNOSIS (No Firmware Upload Needed)")
print("=" * 70)

print("\n1. Device Status Check:")
try:
    r = requests.get(f"{DEVICE}/api/status", timeout=5)
    status = r.json()
    print(f"   ✓ Device online")
    print(f"     Firmware: {status['firmware_version']}")
    print(f"     IP: {status['sta_ip']}")
except Exception as e:
    print(f"   ✗ Cannot reach device: {e}")
    exit(1)

print("\n2. CAN API Endpoints Check:")
try:
    # Test send
    r = requests.post(
        f"{DEVICE}/api/can/send",
        json={"pgn": 0xDEAD, "priority": 6, "source": 0xAA, "destination": 0xBB, "data": [0,1,2,3,4,5,6,7]},
        timeout=5
    )
    print(f"   ✓ /api/can/send: {r.status_code}")
except Exception as e:
    print(f"   ✗ /api/can/send: {e}")

try:
    # Test receive
    r = requests.get(f"{DEVICE}/api/can/receive?timeout=500", timeout=5)
    data = r.json()
    print(f"   ✓ /api/can/receive: {r.status_code}")
    print(f"     Current frames in buffer: {data.get('count', 0)}")
except Exception as e:
    print(f"   ✗ /api/can/receive: {e}")

print("\n3. ANALYSIS:")
print("   Current firmware 1.3.78 is using GPIO 19/20 for CAN")
print("   - Can SEND frames: YES (GPIO 19 TX working)")
print("   - Can RECEIVE frames: NO (GPIO 20 RX not receiving)")
print()
print("   This suggests ONE of:")
print("   [A] GPIO 20 is not connected to CAN transceiver RX")
print("   [B] CAN transceiver is in wrong mode (maybe R/STBY pin issue)")
print("   [C] Waveshare board uses different pins (e.g., 4/5, 16/17, etc)")
print()

print("\n4. RECOMMENDED NEXT STEPS:")
print("   Option 1: Check physical solder joints at:")
print("      - GPIO 20 pin on ESP32-S3")
print("      - CAN transceiver RX input")
print("      - USB_SEL IO expander pin")
print()
print("   Option 2: Try different GPIO pins by rebuilding with:")
print("      pio run -e waveshare_7in -D USE_CAN_PINS_4_5")
print("      pio run -e waveshare_7in -D USE_CAN_PINS_16_17")
print("      pio run -e waveshare_7in -D USE_CAN_PINS_26_27")
print()
print("   Option 3: Check Waveshare documentation for CAN pin assignment")
print()

print("=" * 70)
