#!/usr/bin/env python3
"""
Live test of Output 9 ON/OFF - monitors serial while sending API commands
"""
import serial
import time
import requests
import threading

DEVICE_IP = "192.168.7.116"
SERIAL_PORT = "COM5"
SERIAL_BAUD = 115200

def monitor_serial(duration=5):
    """Monitor serial output for specified duration"""
    print(f"\n[Serial Monitor] Capturing for {duration}s...")
    s = serial.Serial(SERIAL_PORT, SERIAL_BAUD, timeout=0.1)
    start = time.time()
    
    while (time.time() - start) < duration:
        line = s.readline().decode('utf-8', errors='replace').rstrip()
        if line and ('TX' in line or 'Task' in line or 'Output9' in line or 'CanManager' in line):
            print(f"  {line}")
    
    s.close()
    print("[Serial Monitor] Done\n")

def send_api(endpoint, label):
    """Send API request"""
    url = f"http://{DEVICE_IP}/api/infinitybox/{endpoint}"
    print(f"[API] Sending {label}...")
    try:
        r = requests.post(url, timeout=3)
        print(f"[API] Response: {r.json()}")
    except Exception as e:
        print(f"[API] Error: {e}")

# Test sequence
print("=" * 60)
print("INFINITYBOX OUTPUT 9 LIVE TEST")
print("=" * 60)

input("\nPress ENTER to test Output 9 ON...")
send_api("output9/on", "Output 9 ON")
time.sleep(0.5)
monitor_serial(3)

input("Press ENTER to test Output 9 OFF...")
send_api("output9/off", "Output 9 OFF")
time.sleep(0.5)
monitor_serial(3)

print("\n[Test Complete]")
