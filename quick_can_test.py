import requests
import json
import time

DEVICE_IP = "192.168.7.116"
BASE_URL = f"http://{DEVICE_IP}"

print("="*70)
print("CAN BUS HARDWARE TEST - Quick Diagnostics")
print("="*70)

# Get initial status
print("\nCurrent CAN Status:")
print("-" * 70)
resp = requests.get(f"{BASE_URL}/api/can/status", timeout=3)
status = resp.json()
print(f"State: {status.get('state')} (1=RUNNING, 2=BUS_OFF)")
print(f"TX Errors: {status.get('tx_errors')}")
print(f"RX Errors: {status.get('rx_errors')}")
print(f"TX Queue: {status.get('tx_queue')}")
print(f"Bus Off: {status.get('bus_off')}")

# Try a broadcast message
print("\n\nSending broadcast message...")
print("-" * 70)

broadcast = {
    "pgn": 0xFEEB,  # J1939 proprietary B
    "priority": 3,
    "source": 0x63,
    "destination": 0xFF,
    "data": [0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55]
}

resp = requests.post(f"{BASE_URL}/api/can/send", json=broadcast, timeout=3)
print(f"Send result: {resp.json()}")

# Monitor for any echo or response
print("\nListening for 3 seconds...")
time.sleep(0.5)
resp = requests.get(f"{BASE_URL}/api/can/receive?timeout=2500", timeout=4)
data = resp.json()

if data['count'] > 0:
    print(f"✓ Received {data['count']} message(s):")
    for msg in data['messages']:
        data_hex = ' '.join([f"{b:02X}" for b in msg['data']])
        print(f"  ID: 0x{msg['id']} | Data: {data_hex}")
else:
    print("No messages received")

# Final status
print("\n\nFinal CAN Status:")
print("-" * 70)
resp = requests.get(f"{BASE_URL}/api/can/status", timeout=3)
status = resp.json()
print(f"State: {status.get('state')}")
print(f"TX Errors: {status.get('tx_errors')}")
print(f"RX Errors: {status.get('rx_errors')}")
print(f"TX Queue: {status.get('tx_queue')}")

print("\n" + "="*70)
if status.get('tx_errors', 0) > 0:
    print("⚠ TX ERRORS DETECTED")
    print("This usually means:")
    print("  - No CAN termination (120Ω resistor missing)")
    print("  - CAN H/L not connected")
    print("  - Only one device on bus (needs at least 2 + termination)")
elif status.get('tx_queue', 0) > 0:
    print("⚠ MESSAGES QUEUED BUT NOT TRANSMITTED")
    print("This usually means:")
    print("  - Messages sent but no ACK from other devices")
    print("  - POWERCELL not powered or not responding")
    print("  - Wrong baud rate (check POWERCELL is set to 250kbps)")
else:
    print("✓ CAN bus appears healthy")
print("="*70)
