#!/usr/bin/env python3
"""Simple OTA upload via curl-like POST"""
import requests
import sys

try:
    with open(".pio/build/waveshare_7in/firmware.bin", "rb") as f:
        firmware = f.read()
    
    print(f"Uploading {len(firmware)} bytes...")
    
    # Try POST with binary data
    response = requests.post(
        "http://192.168.7.116/api/ota/update",
        data=firmware,
        headers={"Content-Type": "application/octet-stream"},
        timeout=120
    )
    
    print(f"Response: {response.status_code}")
    if response.text:
        print(response.text[:200])
        
except Exception as e:
    print(f"Error: {e}")
    print("\nTrying alternative method...")
    import subprocess
    result = subprocess.run([
        "curl", "-X", "POST", 
        "--data-binary", "@.pio/build/waveshare_7in/firmware.bin",
        "http://192.168.7.116/api/ota/update"
    ], capture_output=True, text=True)
    print(result.stdout)
    if result.stderr:
        print(result.stderr)
