#!/usr/bin/env python3
"""
CAN GPIO PIN TROUBLESHOOTING GUIDE
Comprehensive script to test and fix CAN RX issues on Waveshare ESP32-S3-7"

The issue: CAN RX not working on GPIO 20 (device receives 0 frames)
The solution: Test alternative GPIO pin pairs until one works

Pins to test in order (based on Waveshare demo code and git history):
1. GPIO 15 (TX) / GPIO 4 (RX) - Previously tested in v1.62
2. GPIO 4 (TX) / GPIO 5 (RX)
3. GPIO 16 (TX) / GPIO 17 (RX)
4. GPIO 26 (TX) / GPIO 27 (RX)
"""

import requests
import subprocess
import sys
import time
import json

DEVICE_IP = "192.168.7.116"
BASE_URL = f"http://{DEVICE_IP}"

# Pin configurations to test
PIN_CONFIGS = [
    {"name": "GPIO 15/4 (from v1.62)", "tx": 15, "rx": 4},
    {"name": "GPIO 4/5", "tx": 4, "rx": 5},
    {"name": "GPIO 16/17", "tx": 16, "rx": 17},
    {"name": "GPIO 26/27", "tx": 26, "rx": 27},
    {"name": "GPIO 19/20 (current, NOT working)", "tx": 19, "rx": 20},
]

def test_device_online():
    """Check if device is online"""
    try:
        resp = requests.get(f"{BASE_URL}/api/status", timeout=5)
        return resp.status_code == 200
    except:
        return False

def test_can_rx(timeout_ms=2000):
    """Test if device receives CAN frames"""
    try:
        resp = requests.get(f"{BASE_URL}/api/can/receive?timeout={timeout_ms}", 
                           timeout=timeout_ms//1000+5)
        if resp.status_code == 200:
            frames = resp.json().get('frames', [])
            return len(frames)
        return 0
    except:
        return 0

def send_test_frame():
    """Send a test CAN frame"""
    try:
        test_data = {
            "pgn": 61445,
            "priority": 6,
            "source": 0,
            "destination": 255,
            "data": [0xAA, 0xBB, 0xCC, 0xDD, 0x11, 0x22, 0x33, 0x44]
        }
        resp = requests.post(f"{BASE_URL}/api/can/send", json=test_data, timeout=5)
        return resp.status_code == 200
    except:
        return False

def build_firmware(tx_pin, rx_pin):
    """Build firmware with specified GPIO pins"""
    print(f"\n[BUILD] Compiling firmware for TX=GPIO{tx_pin}, RX=GPIO{rx_pin}")
    print("  This may take 2-5 minutes...")
    
    # Update can_manager.h with new pins
    # For now, we'll just instruct the user
    print(f"\n  TODO: Update platformio.ini or can_manager.h with new GPIO pins")
    print(f"  Then run: pio run -e waveshare_7in")
    
    return False  # Not implemented - requires actual compilation

def upload_firmware_ota(fw_path):
    """Upload firmware via OTA endpoint"""
    print(f"\n[UPLOAD] Uploading firmware via OTA...")
    try:
        with open(fw_path, 'rb') as f:
            files = {'file': f}
            resp = requests.post(f"{BASE_URL}/api/ota/upload", files=files, timeout=300)
        return resp.status_code == 200
    except Exception as e:
        print(f"  Error: {e}")
        return False

def main():
    print("=" * 70)
    print("CAN RX TROUBLESHOOTING - GPIO PIN DISCOVERY")
    print("=" * 70)
    
    # Step 1: Check device is online
    print("\n[STEP 1] Checking device connectivity...")
    if not test_device_online():
        print("ERROR: Device is not online at {BASE_URL}")
        print("Make sure it's connected to WiFi and accessible")
        sys.exit(1)
    
    try:
        resp = requests.get(f"{BASE_URL}/api/status", timeout=5)
        status = resp.json()
        print(f"  ✓ Device online: v{status.get('firmware_version')}")
        print(f"  ✓ IP: {status.get('sta_ip')}")
    except:
        pass
    
    # Step 2: Current status  
    print("\n[STEP 2] Testing current configuration (GPIO 19/20)...")
    print("  Sending test frame...")
    if send_test_frame():
        print("    ✓ Frame sent successfully")
    else:
        print("    ✗ Frame send failed")
    
    time.sleep(0.5)
    print("  Listening for frames...")
    frames_rx = test_can_rx(3000)
    print(f"    📊 Frames received: {frames_rx}")
    
    if frames_rx > 0:
        print("\n✅ SUCCESS! CAN RX is working on GPIO 19/20!")
        print("   No need to test alternative pins.")
        return 0
    
    print("\n❌ No frames received on GPIO 19/20")
    
    # Step 3: Build and test alternatives
    print("\n[STEP 3] Next steps to test alternative GPIO pins...")
    print("\nTo fix this, you need to:")
    print("1. Rebuild firmware with alternative GPIO pins")
    print("2. Upload the new firmware to the device")
    print("3. Test CAN RX with the new pins")
    
    print("\n[SOLUTION] Follow these steps:")
    print("=" * 70)
    print("""
A. Modify can_manager.h or platformio.ini to use alternative pins:

   Option 1: Edit src/can_manager.h and change DEFAULT_TX_PIN/DEFAULT_RX_PIN
   Option 2: Add -D USE_CAN_PINS_15_4 to platformio.ini build_flags

B. Rebuild firmware:
   
   cd D:\Software\Bronco-Controls-4
   pio run -e waveshare_7in

C. Upload new firmware (choose one):
   
   Option A (OTA via web):
   python upload_fw.py
   
   Option B (Direct via serial bootloader):
   python direct_upload.py
   (Then follow on-screen instructions to put device in bootloader mode)

D. Test with this script:
   
   python can_rx_troubleshoot.py

E. If still not working, try next GPIO pair (4/5, then 16/17, etc.)

Recommended test order (based on historical attempts):
  1. GPIO 15/4 (TX/RX) - Previously tested in production
  2. GPIO 4/5
  3. GPIO 16/17
  4. GPIO 26/27
""")
    print("=" * 70)
    
    return 1

if __name__ == "__main__":
    sys.exit(main())
