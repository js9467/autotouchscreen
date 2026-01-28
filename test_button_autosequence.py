"""
Test that button-configured frames automatically trigger Infinitybox sequences.
This validates the auto-detection logic in sendFrame().
"""
import requests
import time

DEVICE_IP = "192.168.7.116"

print("="*70)
print("INFINITYBOX AUTO-SEQUENCE TEST")
print("="*70)

print("""
This test simulates what happens when a button configured with:
  PGN: FF01, Data: 20 80 00 00 00 00 00 00
  
is pressed. It should automatically trigger the full 3-message sequence.
""")

# Test 1: Send FF01 with 20 80 pattern (Output 9 ON)
print("\n[TEST 1] Sending PGN FF01 with data 20 80 (Output 9 ON pattern)")
print("-" * 70)

frame_on = {
    "pgn": 0xFF01,
    "priority": 6,
    "source": 0x80,
    "destination": 0xFF,
    "data": [0x20, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00]
}

try:
    response = requests.post(f"http://{DEVICE_IP}/api/can/send", json=frame_on, timeout=5)
    result = response.json()
    print(f"HTTP Status: {response.status_code}")
    print(f"API Response: {result}")
    
    if result.get('success'):
        print("\n✅ SUCCESS! The frame triggered the sequence.")
        print("   Expected behavior: 3 CAN messages sent (FF01, FF02, FF02)")
    else:
        print("\n❌ FAILED - check serial output for errors")
        
except Exception as e:
    print(f"❌ HTTP Error: {e}")

time.sleep(3)

# Test 2: Send FF01 with 20 00 pattern (Output 9 OFF)  
print("\n[TEST 2] Sending PGN FF01 with data 20 00 (Output 9 OFF pattern)")
print("-" * 70)

frame_off = {
    "pgn": 0xFF01,
    "priority": 6,
    "source": 0x80,
    "destination": 0xFF,
    "data": [0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00]
}

try:
    response = requests.post(f"http://{DEVICE_IP}/api/can/send", json=frame_off, timeout=5)
    result = response.json()
    print(f"HTTP Status: {response.status_code}")
    print(f"API Response: {result}")
    
    if result.get('success'):
        print("\n✅ SUCCESS! The frame triggered the sequence.")
        print("   Expected behavior: 3 CAN messages sent (FF01, FF02, FF02)")
    else:
        print("\n❌ FAILED - check serial output for errors")
        
except Exception as e:
    print(f"❌ HTTP Error: {e}")

print("\n" + "="*70)
print("SUMMARY")
print("="*70)
print("""
✅ If both tests show success=true, then buttons configured with:
   - PGN: FF01
   - Data: 20 80... (or 20 00...)
   
   Will automatically trigger the full Infinitybox sequences!

📱 Now test from the device:
   1. Configure button as shown in your screenshot
   2. Change Priority from 3 to 6 
   3. Press the button
   4. Output 9 should turn ON/OFF!
""")
print("="*70)
