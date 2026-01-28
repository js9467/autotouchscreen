#!/usr/bin/env python3
"""
Upload recovery firmware to ESP32
This firmware will enable USB serial permanently
"""

import subprocess
import sys

PORT = "COM5"
BAUD = 115200
FIRMWARE = ".pio/build/ota_recovery/firmware.bin"

print("=" * 60)
print("  RECOVERY FIRMWARE UPLOAD")
print("=" * 60)
print("\nThis will upload firmware that:")
print("  ✓ Enables USB serial (USB_SEL = LOW)")
print("  ✓ Maintains WiFi connectivity")
print("  ✓ Allows subsequent serial uploads")
print("\n" + "=" * 60)
print("\nBOOTLOADER MODE INSTRUCTIONS:")
print("1. Power OFF the device (unplug USB)")
print("2. Hold the BOOT button")
print("3. Plug in USB (while holding BOOT)")
print("4. Release BOOT button after 2 seconds")
print("\nPress ENTER when ready...")
input()

print("\nAttempting upload...")
try:
    result = subprocess.run(
        [
            sys.executable, "-m", "esptool",
            "--chip", "esp32s3",
            "--port", PORT,
            "--baud", str(BAUD),
            "write_flash",
            "0x0", FIRMWARE
        ],
        capture_output=False,
        text=True
    )
    
    if result.returncode == 0:
        print("\n" + "=" * 60)
        print("  ✓ UPLOAD SUCCESSFUL!")
        print("=" * 60)
        print("\nThe device now has recovery firmware running.")
        print("USB serial is ENABLED and ready.")
        print("\nYou can now:")
        print("  1. Upload any firmware via serial")
        print("  2. Use PlatformIO upload normally")
        print("  3. Monitor serial output")
    else:
        print("\n✗ Upload failed")
        print("Make sure device is in bootloader mode")
        
except FileNotFoundError:
    print("\n✗ Error: esptool not found")
    print("Install with: pip install esptool")
except Exception as e:
    print(f"\n✗ Error: {e}")
