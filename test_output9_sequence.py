"""
Test Output 9 ON/OFF sequences to verify correct message order and timing.
This validates the CAN frame sequences match the Infinitybox requirements.
"""
import requests
import serial
import time
import threading

DEVICE_IP = "192.168.7.116"
SERIAL_PORT = "COM5"
SERIAL_BAUD = 115200

print("="*70)
print("OUTPUT 9 SEQUENCE VALIDATION TEST")
print("="*70)

# Expected sequences
expected_on = [
    "FF01 with data: 20 80 00 00 00 00 00 00",
    "FF02 with data: 80 00 00 00 00 00 00 00",
    "FF02 with data: 00 00 00 00 00 00 00 00"
]

expected_off = [
    "FF01 with data: 20 00 00 00 00 00 00 00",
    "FF02 with data: 80 00 00 00 00 00 00 00",
    "FF02 with data: 00 00 00 00 00 00 00 00"
]

print("\n📋 EXPECTED SEQUENCES:")
print("\nOutput 9 ON (3 messages with 10ms delays):")
for i, msg in enumerate(expected_on, 1):
    print(f"  {i}. {msg}")

print("\nOutput 9 OFF (3 messages with 10ms delays):")
for i, msg in enumerate(expected_off, 1):
    print(f"  {i}. {msg}")

# Open serial port
print(f"\n🔌 Opening serial port {SERIAL_PORT}...")
try:
    ser = serial.Serial(SERIAL_PORT, SERIAL_BAUD, timeout=0.5)
    ser.reset_input_buffer()
    print("   ✓ Serial connected")
except Exception as e:
    print(f"   ✗ Failed: {e}")
    exit(1)

# Monitor serial in background
serial_lines = []
stop_monitoring = False

def monitor_serial():
    while not stop_monitoring:
        try:
            if ser.in_waiting:
                line = ser.readline().decode('utf-8', errors='replace').strip()
                if line:
                    serial_lines.append((time.time(), line))
        except:
            pass
        time.sleep(0.001)

monitor_thread = threading.Thread(target=monitor_serial, daemon=True)
monitor_thread.start()

# Test Output 9 ON
print("\n🧪 TEST 1: Output 9 ON")
print("=" * 70)
serial_lines.clear()
time.sleep(0.5)

try:
    response = requests.post(f"http://{DEVICE_IP}/api/infinitybox/output9/on", timeout=5)
    print(f"HTTP Status: {response.status_code}")
    result = response.json()
    print(f"API Response: {result}")
    
    if result.get('success'):
        print("✓ API reports success")
    else:
        print("✗ API reports failure")
except Exception as e:
    print(f"✗ HTTP error: {e}")

time.sleep(2)

# Analyze ON sequence
print("\n📊 Serial Output Analysis:")
tx_messages = [l for t, l in serial_lines if 'TX' in l or 'PGN' in l]
if tx_messages:
    print(f"Found {len(tx_messages)} TX-related messages:")
    for msg in tx_messages[:10]:
        print(f"  {msg}")
else:
    print("⚠ No TX messages found in serial output")

# Test Output 9 OFF
print("\n🧪 TEST 2: Output 9 OFF")
print("=" * 70)
serial_lines.clear()
time.sleep(0.5)

try:
    response = requests.post(f"http://{DEVICE_IP}/api/infinitybox/output9/off", timeout=5)
    print(f"HTTP Status: {response.status_code}")
    result = response.json()
    print(f"API Response: {result}")
    
    if result.get('success'):
        print("✓ API reports success")
    else:
        print("✗ API reports failure")
except Exception as e:
    print(f"✗ HTTP error: {e}")

time.sleep(2)

# Analyze OFF sequence
print("\n📊 Serial Output Analysis:")
tx_messages = [l for t, l in serial_lines if 'TX' in l or 'PGN' in l]
if tx_messages:
    print(f"Found {len(tx_messages)} TX-related messages:")
    for msg in tx_messages[:10]:
        print(f"  {msg}")
else:
    print("⚠ No TX messages found in serial output")

# Cleanup
stop_monitoring = True
time.sleep(0.2)
ser.close()

print("\n" + "="*70)
print("SUMMARY")
print("="*70)
print("""
✅ If you see verbose TX Frame messages with the correct data bytes,
   the sequences are working properly!

✅ Expected to see 3 messages per button press with 10ms spacing.

⚠  If no TX messages appear, the serial logging may be disabled or
   the CAN transceiver is not enabled (check USB_SEL pin).

🔧 Next steps:
   - Connect a CAN analyzer to verify messages on the bus
   - Test with actual Infinitybox hardware
   - Verify output 9 responds correctly
""")
print("="*70)
