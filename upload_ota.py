#!/usr/bin/env python3
"""Upload firmware via OTA to device"""
import requests
import time
import sys

DEVICE_IP = "192.168.7.116"
FIRMWARE_PATH = ".pio/build/waveshare_7in/firmware.bin"

print("=" * 70)
print("  OTA FIRMWARE UPLOAD")
print("=" * 70)

# Read firmware
try:
    with open(FIRMWARE_PATH, 'rb') as f:
        firmware_data = f.read()
    print(f"\n✓ Loaded firmware: {len(firmware_data)} bytes")
except Exception as e:
    print(f"✗ Failed to load firmware: {e}")
    sys.exit(1)

# Upload
print(f"\nUploading to {DEVICE_IP}...")
print("(This may take 1-2 minutes)\n")

try:
    url = f"http://{DEVICE_IP}/api/ota/update"
    files = {'firmware': ('firmware.bin', firmware_data)}
    
    response = requests.post(url, files=files, timeout=180)
    
    if response.status_code == 200:
        print("✓ Upload successful!")
        print("\nWaiting for device to reboot...")
        time.sleep(10)
        
        # Check if device is back
        try:
            resp = requests.get(f"http://{DEVICE_IP}/api/status", timeout=10)
            status = resp.json()
            print(f"\n✓ Device rebooted successfully!")
            print(f"  Firmware version: {status.get('firmware_version')}")
            print(f"  IP: {status.get('sta_ip')}")
        except:
            print("\nDevice may still be rebooting... check manually in ~30 seconds")
    else:
        print(f"✗ Upload failed with status {response.status_code}")
        print(f"Response: {response.text}")
        
except Exception as e:
    print(f"✗ Error during upload: {e}")
    sys.exit(1)

print("\n" + "=" * 70)
