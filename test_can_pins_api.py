#!/usr/bin/env python3
"""
Test different GPIO pin configurations for CAN via Web API.
Since the device is already running and the web API is working,
we can modify the configuration dynamically without rebuilding.
"""

import requests
import time
import json
import sys

DEVICE_IP = "192.168.7.116"
BASE_URL = f"http://{DEVICE_IP}"

def get_status():
    """Get device status"""
    try:
        resp = requests.get(f"{BASE_URL}/api/status", timeout=5)
        if resp.status_code == 200:
            return resp.json()
        else:
            print(f"❌ Status check failed: {resp.status_code}")
            return None
    except Exception as e:
        print(f"❌ Cannot reach device: {e}")
        return None

def test_can_receive(timeout_ms=2000):
    """Test if device receives any CAN frames"""
    try:
        resp = requests.get(f"{BASE_URL}/api/can/receive?timeout={timeout_ms}", timeout=timeout_ms//1000+5)
        if resp.status_code == 200:
            data = resp.json()
            frames = data.get('frames', [])
            return len(frames)
        else:
            print(f"  ❌ CAN receive failed: {resp.status_code}")
            return 0
    except Exception as e:
        print(f"  ❌ Cannot test receive: {e}")
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
        if resp.status_code == 200:
            print(f"  ✓ Sent test frame: {resp.json()}")
            return True
        else:
            print(f"  ❌ Send failed: {resp.status_code}")
            return False
    except Exception as e:
        print(f"  ❌ Cannot send frame: {e}")
        return False

def main():
    print("=" * 60)
    print("CAN GPIO PIN TESTER (Web API)")
    print("=" * 60)
    
    # Check device is online
    print("\n[1] Checking device status...")
    status = get_status()
    if not status:
        print("ERROR: Device is not responding. Make sure it's online at 192.168.7.116")
        sys.exit(1)
    
    print(f"  ✓ Device online: v{status.get('firmware_version', 'unknown')}")
    print(f"  ✓ WiFi IP: {status.get('sta_ip', 'unknown')}")
    
    # Test current pin configuration
    print("\n[2] Testing CURRENT CAN configuration (GPIO 19/20)...")
    print(f"    Sending test frame...")
    send_test_frame()
    time.sleep(0.5)
    
    print(f"    Listening for 3 seconds...")
    frame_count = test_can_receive(3000)
    print(f"    📊 Frames received: {frame_count}")
    
    if frame_count > 0:
        print("\n✅ SUCCESS! CAN RX is working on GPIO 19/20!")
        print("    No need to test alternative pins.")
        return
    
    print("\n❌ No frames received on GPIO 19/20")
    print("\nNote: To test alternative GPIO pins (4/5, 16/17, 26/27),")
    print("you would need to rebuild and upload firmware with those pins.")
    print("\nThe web API cannot change GPIO pins at runtime - that requires recompilation.")
    print("\nNext steps:")
    print("  1. Rebuild with -D USE_CAN_PINS_4_5")
    print("  2. Upload new firmware")
    print("  3. Run this test again to verify")

if __name__ == "__main__":
    main()
