#!/usr/bin/env python3
"""
Direct OTA firmware upload via HTTP
"""

import requests
import sys
from pathlib import Path

# Configuration
DEVICE_IP = "192.168.7.116"  # Change if needed
FIRMWARE_PATH = ".pio/build/ota_recovery/firmware.bin"
OTA_ENDPOINT = f"http://{DEVICE_IP}/api/ota/update"

def upload_ota():
    print("=" * 60)
    print("  OTA RECOVERY FIRMWARE UPLOAD")
    print("=" * 60)
    
    # Check firmware exists
    if not Path(FIRMWARE_PATH).exists():
        print(f"✗ Firmware not found: {FIRMWARE_PATH}")
        print("Run: pio run -e ota_recovery")
        return False
    
    fw_size = Path(FIRMWARE_PATH).stat().st_size
    print(f"\n✓ Firmware found: {FIRMWARE_PATH}")
    print(f"  Size: {fw_size:,} bytes ({fw_size/1024:.1f} KB)")
    
    # Check device is reachable
    print(f"\n⏳ Checking device at {DEVICE_IP}...")
    try:
        response = requests.get(f"http://{DEVICE_IP}/api/status", timeout=3)
        print(f"✓ Device is online")
        print(f"  Response: {response.text[:100]}")
    except requests.exceptions.RequestException as e:
        print(f"✗ Device not reachable: {e}")
        print("\nTroubleshooting:")
        print(f"  1. Is device at {DEVICE_IP}? Check your router/device display")
        print("  2. Is device on same network?")
        print("  3. Is WiFi connected?")
        return False
    
    # Upload firmware
    print(f"\n⏳ Uploading firmware to {OTA_ENDPOINT}...")
    try:
        with open(FIRMWARE_PATH, 'rb') as f:
            files = {'file': ('firmware.bin', f, 'application/octet-stream')}
            response = requests.post(OTA_ENDPOINT, files=files, timeout=60)
        
        if response.status_code == 200:
            print("\n" + "=" * 60)
            print("  ✓ OTA UPDATE SUCCESSFUL!")
            print("=" * 60)
            print("\nDevice is rebooting with recovery firmware...")
            print("Once rebooted:")
            print("  ✓ USB serial will be ENABLED")
            print("  ✓ You can upload via USB normally")
            print("  ✓ Serial monitoring will work")
            print(f"\nWait 10 seconds, then check: http://{DEVICE_IP}")
            return True
        else:
            print(f"\n✗ Upload failed: {response.status_code}")
            print(f"Response: {response.text}")
            return False
            
    except requests.exceptions.RequestException as e:
        print(f"\n✗ Upload error: {e}")
        return False

if __name__ == "__main__":
    # Allow IP override
    if len(sys.argv) > 1:
        DEVICE_IP = sys.argv[1]
        OTA_ENDPOINT = f"http://{DEVICE_IP}/api/ota/update"
    
    success = upload_ota()
    sys.exit(0 if success else 1)
