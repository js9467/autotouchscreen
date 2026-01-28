import requests
import json
import time

DEVICE_IP = "192.168.7.116"
BASE_URL = f"http://{DEVICE_IP}"

print("="*70)
print("COMPLETE CAN BUS DIAGNOSTIC & TEST SUITE")
print("="*70)

# Step 1: Check CAN bus status
print("\n1. Checking CAN bus initialization...")
print("-" * 70)
try:
    resp = requests.get(f"{BASE_URL}/api/can/status", timeout=3)
    status = resp.json()
    print(f"Response: {json.dumps(status, indent=2)}")
    
    if not status.get('ready'):
        print("\n⚠ CAN bus not initialized!")
        print("The device may need a reboot.")
        exit(1)
        
    if status.get('bus_off'):
        print("\n⚠ CAN bus is in BUS-OFF state!")
        print("This usually means:")
        print("  - No termination resistor")
        print("  - Wiring issue (H/L swapped or disconnected)")
        print("  - Wrong baud rate")
        
    if status.get('tx_errors', 0) > 100 or status.get('rx_errors', 0) > 100:
        print(f"\n⚠ High error count: TX={status.get('tx_errors')}, RX={status.get('rx_errors')}")
        print("Check CAN wiring and termination")
        
    print("\n✓ CAN bus is ready")
    
except Exception as e:
    print(f"✗ Error: {e}")
    exit(1)

# Step 2: Test sending a simple message
print("\n2. Testing CAN transmit...")
print("-" * 70)

test_frame = {
    "pgn": 0xFF41,
    "priority": 6,
    "source": 0x63,
    "destination": 0x01,
    "data": [0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00]
}

try:
    resp = requests.post(f"{BASE_URL}/api/can/send", json=test_frame, timeout=3)
    result = resp.json()
    print(f"Send result: {json.dumps(result, indent=2)}")
    
    if not result.get('success'):
        print("\n✗ Failed to send CAN message!")
        print("Checking bus status again...")
        resp = requests.get(f"{BASE_URL}/api/can/status", timeout=3)
        status = resp.json()
        print(f"Status: {json.dumps(status, indent=2)}")
    else:
        print("✓ Message sent successfully")
        
except Exception as e:
    print(f"✗ Error: {e}")

# Step 3: Monitor for received messages
print("\n3. Listening for CAN responses (2 seconds)...")
print("-" * 70)

try:
    resp = requests.get(f"{BASE_URL}/api/can/receive?timeout=2000", timeout=4)
    data = resp.json()
    
    if data['count'] > 0:
        print(f"✓ Received {data['count']} message(s)!")
        for i, msg in enumerate(data['messages'], 1):
            data_hex = ' '.join([f"{b:02X}" for b in msg['data']])
            print(f"  {i}. ID: 0x{msg['id']} | Data: {data_hex}")
    else:
        print("No messages received (this is expected if POWERCELL isn't responding)")
        
except Exception as e:
    print(f"✗ Error: {e}")

# Step 4: Extended scan of all addresses
print("\n4. Scanning all POWERCELL addresses (1-16)...")
print("-" * 70)

found_devices = []
for addr in range(1, 17):
    poll_frame = {
        "pgn": 0xFF41,
        "priority": 6,
        "source": 0x63,
        "destination": addr,
        "data": [0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00]
    }
    
    try:
        # Send
        resp = requests.post(f"{BASE_URL}/api/can/send", json=poll_frame, timeout=2)
        if not resp.json().get('success'):
            print(f"  Address {addr:2d}: ✗ send failed")
            continue
            
        # Listen
        time.sleep(0.1)
        resp = requests.get(f"{BASE_URL}/api/can/receive?timeout=200", timeout=1)
        data = resp.json()
        
        if data['count'] > 0:
            print(f"  Address {addr:2d}: ✓ FOUND! ({data['count']} msg)")
            found_devices.append(addr)
            for msg in data['messages']:
                data_hex = ' '.join([f"{b:02X}" for b in msg['data']])
                print(f"             ID: 0x{msg['id']} | Data: {data_hex}")
        else:
            print(f"  Address {addr:2d}: no response", end='\r')
            
    except Exception as e:
        print(f"  Address {addr:2d}: error - {str(e)[:30]}")

print()  # Clear the \r line
if found_devices:
    print(f"\n✓ Found devices at addresses: {found_devices}")
else:
    print("\n✗ No devices responded")

# Step 5: Passive monitoring
print("\n5. Passive monitoring (5 seconds)...")
print("-" * 70)
print("Listening for any CAN bus activity...")

try:
    resp = requests.get(f"{BASE_URL}/api/can/receive?timeout=5000", timeout=7)
    data = resp.json()
    
    if data['count'] > 0:
        print(f"\n✓ Detected {data['count']} message(s) on bus!")
        for i, msg in enumerate(data['messages'], 1):
            data_hex = ' '.join([f"{b:02X}" for b in msg['data']])
            print(f"  {i}. ID: 0x{msg['id']:08X} | Time: {msg['timestamp']}ms | Data: {data_hex}")
    else:
        print("No activity detected")
        
except Exception as e:
    print(f"✗ Error: {e}")

# Final status check
print("\n6. Final CAN bus status...")
print("-" * 70)
try:
    resp = requests.get(f"{BASE_URL}/api/can/status", timeout=3)
    status = resp.json()
    print(f"State: {status.get('state')}")
    print(f"TX Errors: {status.get('tx_errors')}")
    print(f"RX Errors: {status.get('rx_errors')}")
    print(f"TX Queue: {status.get('tx_queue')}")
    print(f"RX Queue: {status.get('rx_queue')}")
    
except Exception as e:
    print(f"✗ Error: {e}")

print("\n" + "="*70)
print("DIAGNOSTIC COMPLETE")
print("="*70)

if found_devices:
    print(f"✓ SUCCESS: POWERCELL NGX devices found at addresses: {found_devices}")
else:
    print("✗ No POWERCELL NGX devices responded")
    print("\nTroubleshooting checklist:")
    print("  [ ] POWERCELL has power (check LED)")
    print("  [ ] CAN H connected to correct pin")
    print("  [ ] CAN L connected to correct pin")  
    print("  [ ] 120Ω termination resistor installed")
    print("  [ ] CAN transceiver powered (3.3V or 5V)")
    print("  [ ] ESP32 GPIO 19/20 connected to transceiver RX/TX")
    print("  [ ] POWERCELL configured for J1939 protocol")
    print("  [ ] Baud rate is 250kbps on both devices")
