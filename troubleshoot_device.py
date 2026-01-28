#!/usr/bin/env python3
"""
Comprehensive ESP32 device troubleshooting script.
Helps diagnose communication, driver, and firmware issues.
"""

import subprocess
import serial
import serial.tools.list_ports
import time
import sys
from pathlib import Path

def print_header(title):
    print(f"\n{'='*60}")
    print(f"  {title}")
    print(f"{'='*60}")

def print_result(status, message):
    symbol = "✓" if status else "✗"
    color = "\033[92m" if status else "\033[91m"  # Green or Red
    reset = "\033[0m"
    print(f"{color}{symbol}{reset} {message}")

def check_usb_devices():
    """Check for USB devices using wmic on Windows"""
    print_header("1. USB DEVICE DETECTION")
    try:
        result = subprocess.run(
            ["wmic", "logicaldisk", "get", "name"],
            capture_output=True,
            text=True,
            timeout=5
        )
        print("Testing WMIC... OK")
        
        # List COM ports via wmic
        result = subprocess.run(
            ["wmic", "path", "win32_serialportconfiguration", "get", "name"],
            capture_output=True,
            text=True,
            timeout=5
        )
        print("\nSerialPort configurations:")
        print(result.stdout)
        
    except Exception as e:
        print(f"WMIC check failed: {e}")

def check_serial_ports():
    """Enumerate all available serial ports"""
    print_header("2. SERIAL PORT ENUMERATION")
    try:
        ports = serial.tools.list_ports.comports()
        if ports:
            print(f"Found {len(ports)} serial port(s):\n")
            for port in ports:
                print(f"  • {port.device}")
                print(f"    Description: {port.description}")
                print(f"    Manufacturer: {port.manufacturer}")
                print(f"    Serial Number: {port.serial_number}")
                print(f"    VID:PID: {port.vid:04X}:{port.pid:04X}\n")
            return [p.device for p in ports]
        else:
            print_result(False, "No serial ports found!")
            return []
    except Exception as e:
        print_result(False, f"Error listing ports: {e}")
        return []

def test_serial_connection(port):
    """Test basic serial connection"""
    print_header(f"3. SERIAL CONNECTION TEST ({port})")
    try:
        ser = serial.Serial(port, 115200, timeout=2)
        print_result(True, f"Port opened: {port}")
        
        # Try to read any boot messages
        time.sleep(1)
        if ser.in_waiting:
            data = ser.read(ser.in_waiting)
            print(f"✓ Data received ({len(data)} bytes):")
            print(f"  {data[:200]}")
            print_result(True, "Device is responsive")
        else:
            print_result(False, "No data received from device (may be in bootloader)")
        
        ser.close()
        return True
    except serial.SerialException as e:
        print_result(False, f"Cannot open port: {e}")
        return False
    except Exception as e:
        print_result(False, f"Error: {e}")
        return False

def test_esptool_detection(port):
    """Test if esptool can detect the device"""
    print_header(f"4. ESPTOOL DETECTION TEST ({port})")
    try:
        result = subprocess.run(
            [sys.executable, "-m", "esptool", "--port", port, "chip_id"],
            capture_output=True,
            text=True,
            timeout=10
        )
        
        if result.returncode == 0:
            print_result(True, "esptool connected and detected chip")
            print(result.stdout)
            return True
        else:
            if "No serial data received" in result.stderr:
                print_result(False, "Device not responding to bootloader commands")
                print("  → Device may not be in bootloader mode")
                print("  → Check USB cable data lines (not just power)")
                print("  → Try holding BOOT button during power-on")
            else:
                print_result(False, f"esptool error: {result.stderr[:200]}")
            return False
    except subprocess.TimeoutExpired:
        print_result(False, "esptool timeout - device not responding")
        return False
    except FileNotFoundError:
        print_result(False, "esptool not installed. Run: pip install esptool")
        return False
    except Exception as e:
        print_result(False, f"Error: {e}")
        return False

def test_bootloader_mode(port):
    """Instructions and test for entering bootloader mode"""
    print_header(f"5. BOOTLOADER MODE TEST")
    print("\nBootloader Entry Procedure:")
    print("1. Power OFF the device completely (unplug USB)")
    print("2. Wait 2 seconds")
    print("3. Hold the BOOT button")
    print("4. Plug in USB cable (while holding BOOT)")
    print("5. Wait 2 seconds")
    print("6. Release BOOT button")
    print("7. Device should now be in bootloader mode")
    print("\nPress ENTER after completing these steps...")
    input()
    
    print("\nTesting esptool connection in bootloader mode...")
    return test_esptool_detection(port)

def test_usb_drivers():
    """Check for USB serial drivers"""
    print_header("6. USB SERIAL DRIVER CHECK")
    try:
        # Check for CH340/CH341 (common USB-to-serial chip)
        result = subprocess.run(
            ["powershell", "-Command", 
             "Get-WmiObject Win32_PnPDevice | Where-Object {$_.Name -match 'CH340|CH341|USB|Serial'} | Select-Object Name"],
            capture_output=True,
            text=True,
            timeout=5
        )
        if result.stdout:
            print("USB/Serial devices found:")
            print(result.stdout)
        
        # Check Device Manager for problematic devices
        result = subprocess.run(
            ["powershell", "-Command",
             "Get-WmiObject Win32_PnPDevice | Where-Object {$_.Status -ne 'OK'} | Select-Object Name,Status"],
            capture_output=True,
            text=True,
            timeout=5
        )
        if result.stdout.strip():
            print_result(False, "Problematic devices found:")
            print(result.stdout)
        else:
            print_result(True, "No problematic USB/Serial devices")
            
    except Exception as e:
        print(f"Driver check error: {e}")

def main():
    print("\n" + "="*60)
    print("  ESP32 DEVICE TROUBLESHOOTING UTILITY")
    print("="*60)
    
    # Step 1: Check USB devices
    check_usb_devices()
    
    # Step 2: Enumerate ports
    ports = check_serial_ports()
    
    if not ports:
        print_result(False, "No serial ports available!")
        print("\nTroubleshooting steps:")
        print("1. Check USB cable is properly connected (both ends)")
        print("2. Try a different USB port")
        print("3. Try a different USB cable (preferably data cable)")
        print("4. Install/update USB drivers for your device")
        print("5. Check Device Manager for 'Unknown Device' or error markers")
        sys.exit(1)
    
    # Step 3: Test each port
    working_port = None
    for port in ports:
        if test_serial_connection(port):
            working_port = port
            break
    
    # Step 4: Test esptool
    if working_port:
        test_esptool_detection(working_port)
        
        # Step 5: Test bootloader mode
        print("\n" + "="*60)
        try_bootloader = input("Try bootloader mode test? (y/n): ").lower().strip()
        if try_bootloader == 'y':
            test_bootloader_mode(working_port)
    
    # Step 6: Check drivers
    test_usb_drivers()
    
    # Summary
    print_header("SUMMARY & NEXT STEPS")
    if working_port and ports:
        print("✓ Device detected on:", working_port)
        print("\nNext steps:")
        print("1. If esptool connected: Try uploading firmware")
        print("2. If esptool failed: Check bootloader mode procedure")
        print("3. If no data received: Check USB cable (data lines, not just power)")
    else:
        print("✗ Device communication issue detected")
        print("\nPriority checks:")
        print("1. Verify USB cable has DATA lines (not power-only)")
        print("2. Try different USB port on computer")
        print("3. Check Device Manager for unknown devices")
        print("4. Update USB drivers")
        print("5. If device shows up but won't communicate:")
        print("   - May have corrupted bootloader")
        print("   - May require factory reset or JTAG recovery")

if __name__ == "__main__":
    main()
