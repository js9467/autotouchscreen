import requests
import json
import time

# Device IP - adjust if needed
DEVICE_IP = "192.168.7.116"
BASE_URL = f"http://{DEVICE_IP}"

print("="*60)
print("CAN Communication Test via Web API")
print("="*60)
print(f"Device: {BASE_URL}\n")

# Test 1: Send CAN poll message to POWERCELL NGX at address 1
print("Test 1: Polling POWERCELL NGX at address 1")
print("-" * 60)

# J1939 Poll message: PGN 0xFF41, source 0x63, destination 0x01
poll_frame = {
    "pgn": 0xFF41,
    "priority": 6,
    "source": 0x63,
    "destination": 0x01,
    "data": [0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00]
}

try:
    print(f"Sending: {json.dumps(poll_frame, indent=2)}")
    response = requests.post(f"{BASE_URL}/api/can/send", json=poll_frame, timeout=5)
    print(f"Response ({response.status_code}): {response.text}\n")
except Exception as e:
    print(f"Error: {e}\n")

# Test 2: Listen for CAN responses
print("Test 2: Listening for CAN responses (3 seconds)")
print("-" * 60)

try:
    response = requests.get(f"{BASE_URL}/api/can/receive?timeout=3000", timeout=5)
    data = response.json()
    print(f"Received {data['count']} messages:")
    for msg in data.get('messages', []):
        data_hex = ' '.join([f"{b:02X}" for b in msg['data']])
        print(f"  ID: 0x{msg['id']} | Data: {data_hex}")
    print()
except Exception as e:
    print(f"Error: {e}\n")

# Test 3: Send configuration to POWERCELL NGX at address 1  
print("Test 3: Sending configuration to POWERCELL NGX address 1")
print("-" * 60)

# J1939 Config message: PGN 0xFF41, command 0x10 (configure)
config_frame = {
    "pgn": 0xFF41,
    "priority": 6,
    "source": 0x63,
    "destination": 0x01,
    "data": [
        0x10,  # Command: Configure
        0x01,  # Address: 1
        0x01,  # Enabled: Yes
        0x00, 0x00, 0x00, 0x00, 0x00
    ]
}

try:
    print(f"Sending: {json.dumps(config_frame, indent=2)}")
    response = requests.post(f"{BASE_URL}/api/can/send", json=config_frame, timeout=5)
    print(f"Response ({response.status_code}): {response.text}\n")
except Exception as e:
    print(f"Error: {e}\n")

# Test 4: Monitor for any traffic
print("Test 4: Extended monitoring (5 seconds)")
print("-" * 60)

try:
    response = requests.get(f"{BASE_URL}/api/can/receive?timeout=5000", timeout=7)
    data = response.json()
    print(f"Total messages received: {data['count']}")
    if data['count'] > 0:
        print("\nAll messages:")
        for i, msg in enumerate(data.get('messages', []), 1):
            data_hex = ' '.join([f"{b:02X}" for b in msg['data']])
            print(f"  {i}. ID: 0x{msg['id']} | Time: {msg['timestamp']} | Data: {data_hex}")
    else:
        print("  No CAN traffic detected")
    print()
except Exception as e:
    print(f"Error: {e}\n")

print("="*60)
print("Test Complete!")
print("="*60)
print("\nIf no messages were received:")
print("  1. Verify CAN wiring (GPIO 19/20)")
print("  2. Check POWERCELL NGX is powered")
print("  3. Confirm CAN bus termination")
print("  4. Try different device addresses (1-16)")
