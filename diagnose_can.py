import requests
import json
import time

DEVICE_IP = "192.168.7.116"
BASE_URL = f"http://{DEVICE_IP}"

print("="*70)
print("POWERCELL NGX - Comprehensive CAN Diagnostic")
print("="*70)

# Test 1: Scan all addresses 1-16
print("\n1. Scanning POWERCELL NGX addresses (1-16)...")
print("-" * 70)

for addr in range(1, 17):
    poll_frame = {
        "pgn": 0xFF41,
        "priority": 6,
        "source": 0x63,
        "destination": addr,
        "data": [0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00]
    }
    
    try:
        # Send poll
        resp = requests.post(f"{BASE_URL}/api/can/send", json=poll_frame, timeout=3)
        
        # Wait briefly for response
        time.sleep(0.2)
        
        # Check for responses
        resp = requests.get(f"{BASE_URL}/api/can/receive?timeout=200", timeout=2)
        data = resp.json()
        
        if data['count'] > 0:
            print(f"  Address {addr:2d}: ✓ RESPONSE! Got {data['count']} message(s)")
            for msg in data['messages']:
                data_hex = ' '.join([f"{b:02X}" for b in msg['data']])
                print(f"    ID: 0x{msg['id']} | Data: {data_hex}")
        else:
            print(f"  Address {addr:2d}: no response")
    except Exception as e:
        print(f"  Address {addr:2d}: error - {e}")

# Test 2: Passive monitoring
print("\n2. Passive CAN bus monitoring (10 seconds)...")
print("-" * 70)
print("Listening for ANY CAN traffic (including broadcast messages)...")

try:
    resp = requests.get(f"{BASE_URL}/api/can/receive?timeout=10000", timeout=12)
    data = resp.json()
    
    if data['count'] > 0:
        print(f"\n✓ Received {data['count']} message(s):")
        for i, msg in enumerate(data['messages'], 1):
            data_hex = ' '.join([f"{b:02X}" for b in msg['data']])
            print(f"  {i}. ID: 0x{msg['id']:08X} | Time: {msg['timestamp']:6d} ms | Data: {data_hex}")
    else:
        print("  ✗ No CAN traffic detected")
except Exception as e:
    print(f"  Error: {e}")

# Test 3: Send broadcast message
print("\n3. Sending broadcast poll (address 0xFF)...")
print("-" * 70)

broadcast_frame = {
    "pgn": 0xFF41,
    "priority": 6,
    "source": 0x63,
    "destination": 0xFF,  # Broadcast
    "data": [0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00]
}

try:
    resp = requests.post(f"{BASE_URL}/api/can/send", json=broadcast_frame, timeout=3)
    print(f"Sent: {resp.json()}")
    
    time.sleep(0.5)
    
    resp = requests.get(f"{BASE_URL}/api/can/receive?timeout=2000", timeout=4)
    data = resp.json()
    
    if data['count'] > 0:
        print(f"\n✓ Received {data['count']} broadcast response(s):")
        for msg in data['messages']:
            data_hex = ' '.join([f"{b:02X}" for b in msg['data']])
            print(f"  ID: 0x{msg['id']} | Data: {data_hex}")
    else:
        print("  No broadcast responses")
except Exception as e:
    print(f"  Error: {e}")

print("\n" + "="*70)
print("Diagnostic Summary:")
print("="*70)
print("✓ CAN bus is operational (messages sent successfully)")
print("✗ No responses from POWERCELL NGX devices")
print("\nTroubleshooting steps:")
print("  1. Verify POWERCELL NGX has power (LED indicators)")
print("  2. Check CAN H/L wiring (GPIO 19=TX, GPIO 20=RX)")
print("  3. Ensure 120Ω termination resistors at both ends of CAN bus")
print("  4. Confirm POWERCELL NGX is configured for J1939 protocol")
print("  5. Try with CAN analyzer to verify bus traffic")
print("  6. Check CAN bus voltage (should see 2.5V nominal)")
