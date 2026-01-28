"""
Diagnose CAN transmission issue by sending test frame and monitoring response.
"""
import serial
import requests
import time
import threading
import json

# Configuration
DEVICE_IP = "192.168.7.116"
SERIAL_PORT = "COM5"
SERIAL_BAUD = 115200

# Test frame (POWERCELL poll request)
test_frame = {
    "pgn": 0xFF01,  # 65281 decimal
    "priority": 6,
    "source": 0x80,
    "destination": 0xFF,
    "data": [0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00]
}

print("="*70)
print("CAN TRANSMISSION DIAGNOSTIC TOOL")
print("="*70)

# Open serial port
print(f"\n[1/4] Opening serial port {SERIAL_PORT}...")
try:
    ser = serial.Serial(SERIAL_PORT, SERIAL_BAUD, timeout=0.5)
    ser.reset_input_buffer()
    print("     ✓ Serial port opened")
except Exception as e:
    print(f"     ✗ Failed to open serial port: {e}")
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
                    serial_lines.append(line)
                    print(f"     [SERIAL] {line}")
        except:
            pass
        time.sleep(0.01)

print("\n[2/4] Starting serial monitor...")
monitor_thread = threading.Thread(target=monitor_serial, daemon=True)
monitor_thread.start()
time.sleep(0.5)
print("     ✓ Monitor started")

# Send test frame via API
print(f"\n[3/4] Sending test CAN frame via HTTP API...")
print(f"     URL: http://{DEVICE_IP}/api/can/send")
print(f"     Frame: PGN=0x{test_frame['pgn']:04X}, Priority={test_frame['priority']}, SA=0x{test_frame['source']:02X}")
print(f"     Data: {' '.join(f'{b:02X}' for b in test_frame['data'])}")

try:
    response = requests.post(
        f"http://{DEVICE_IP}/api/can/send",
        json=test_frame,
        timeout=5
    )
    print(f"     HTTP Status: {response.status_code}")
    result = response.json()
    print(f"     Response: {json.dumps(result, indent=6)}")
    
    if result.get('success'):
        print("     ✓ API reports SUCCESS")
    else:
        print("     ✗ API reports FAILURE")
        
except Exception as e:
    print(f"     ✗ HTTP request failed: {e}")

# Wait for serial output
print("\n[4/4] Waiting for serial output (5 seconds)...")
time.sleep(5)

# Stop monitoring
stop_monitoring = True
time.sleep(0.2)
ser.close()

# Analysis
print("\n" + "="*70)
print("ANALYSIS")
print("="*70)

can_init_msgs = [l for l in serial_lines if '[CAN]' in l or 'TWAI' in l or 'CanManager' in l]
can_tx_msgs = [l for l in serial_lines if 'TX Frame' in l or 'TX SUCCESS' in l or 'TX FAILED' in l]
can_error_msgs = [l for l in serial_lines if 'not initialized' in l or 'failed' in l.lower() or 'error' in l.lower()]

if can_init_msgs:
    print("\n✓ CAN Initialization Messages:")
    for msg in can_init_msgs[:5]:
        print(f"  {msg}")
else:
    print("\n⚠ No CAN initialization messages (device already booted)")

if can_tx_msgs:
    print("\n✓ CAN Transmission Messages:")
    for msg in can_tx_msgs:
        print(f"  {msg}")
else:
    print("\n✗ NO CAN TRANSMISSION MESSAGES FOUND!")
    print("  This indicates CAN frames are NOT being sent.")

if can_error_msgs:
    print("\n⚠ Error Messages:")
    for msg in can_error_msgs[:10]:
        print(f"  {msg}")

print("\n" + "="*70)
print("DIAGNOSIS:")
print("="*70)

if not can_tx_msgs:
    print("""
The CAN frame was NOT transmitted. Possible causes:

1. CanManager::sendFrame() is returning false
2. TWAI driver not initialized (ready_ = false)
3. TWAI transmit is failing
4. Logging not working

Check the serial output above for:
- "[CanManager] TWAI bus not initialized"
- "[CanManager] ✗ TX FAILED"
- Bus state errors
""")
else:
    print("\n✓ CAN transmission appears to be working!")

print("="*70)
