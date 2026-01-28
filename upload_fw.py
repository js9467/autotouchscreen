#!/usr/bin/env python3
"""Upload firmware binary directly to device"""
import os
import time

firmware_path = ".pio/build/waveshare_7in/firmware.bin"
device_ip = "192.168.7.116"

if not os.path.exists(firmware_path):
    print(f"ERROR: {firmware_path} not found!")
    exit(1)

file_size = os.path.getsize(firmware_path)
print(f"Firmware: {file_size} bytes")
print(f"Device: http://{device_ip}")
print()

# Use system curl command for upload
import subprocess

cmd = [
    "curl", "-v",
    "-X", "POST",
    "--data-binary", f"@{firmware_path}",
    f"http://{device_ip}/api/ota/update"
]

print("Uploading...")
result = subprocess.run(cmd, capture_output=True, text=True, timeout=300)

if "200" in result.stdout or "200" in result.stderr:
    print("✓ Upload may have succeeded")
    print("\nWaiting for device to reboot...")
    time.sleep(15)
    
    # Check if device is back
    check_cmd = ["curl", "-s", f"http://{device_ip}/api/status"]
    try:
        check = subprocess.run(check_cmd, capture_output=True, text=True, timeout=10)
        if "firmware_version" in check.stdout:
            print("✓ Device is online")
            print(check.stdout[:200])
    except:
        print("Device may still be rebooting...")
else:
    print("✗ Upload may have failed")
    print("\nSTDOUT:")
    print(result.stdout[-500:] if len(result.stdout) > 500 else result.stdout)
    print("\nSTDERR:")
    print(result.stderr[-500:] if len(result.stderr) > 500 else result.stderr)
