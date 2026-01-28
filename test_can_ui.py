"""
Quick test to send a CAN frame to the device and check serial output.
This simulates what happens when a button is configured with CAN in the UI.
"""

import serial
import time
import json

# Open serial connection
print("Opening serial connection to COM5...")
ser = serial.Serial('COM5', 115200, timeout=1)
time.sleep(0.5)

# Clear any existing data
ser.reset_input_buffer()

print("\n=== Testing CAN Frame Transmission ===")
print("This simulates what happens when you press a button configured with CAN")
print("\nWatching serial output for 15 seconds...")
print("Look for messages like:")
print("  [CanManager] TX Frame: ID=0x...")
print("  [CanManager] ✓ TX SUCCESS")
print("\n" + "="*60 + "\n")

# Monitor serial output
start_time = time.time()
can_messages_found = []
init_messages = []

while time.time() - start_time < 15:
    if ser.in_waiting:
        try:
            line = ser.readline().decode('utf-8', errors='replace').strip()
            if line:
                print(line)
                
                # Track CAN-related messages
                if '[CAN]' in line or '[CanManager]' in line:
                    if 'Initializing TWAI' in line or 'TWAI bus ready' in line or 'TWAI not initialized' in line:
                        init_messages.append(line)
                    if 'TX Frame' in line or 'TX SUCCESS' in line or 'TX FAILED' in line:
                        can_messages_found.append(line)
        except Exception as e:
            pass
    time.sleep(0.01)

print("\n" + "="*60)
print("\n=== ANALYSIS ===\n")

if init_messages:
    print("CAN Initialization Messages Found:")
    for msg in init_messages:
        print(f"  ✓ {msg}")
else:
    print("⚠ No CAN initialization messages found (device may have already booted)")

if can_messages_found:
    print("\nCAN Transmission Messages Found:")
    for msg in can_messages_found:
        print(f"  ✓ {msg}")
else:
    print("\n⚠ No CAN transmission messages found")
    print("   This means:")
    print("   1. No buttons were pressed during monitoring, OR")
    print("   2. CAN frames aren't being sent when buttons are pressed")

print("\n" + "="*60)
print("\nNEXT STEPS:")
print("1. Connect to device WiFi AP (usually 'BroncoControls')")
print("2. Open http://192.168.4.250 in browser")
print("3. Configure a button with CAN frame (e.g., PGN=0xFF01, data=[0x11,0x00,...])")
print("4. Press the button on the touchscreen")
print("5. Watch the serial output above for TX messages")

ser.close()
