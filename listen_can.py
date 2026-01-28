import requests
import time

DEVICE_IP = "192.168.7.116"
BASE_URL = f"http://{DEVICE_IP}"

print("="*70)
print("PASSIVE CAN BUS MONITOR - Listening for POWERCELL Activity")
print("="*70)
print("\nThis will listen for 30 seconds without sending anything.")
print("If POWERCELL is powered and transmitting, we'll see its messages.\n")
print("Listening...")
print("-" * 70)

try:
    resp = requests.get(f"{BASE_URL}/api/can/receive?timeout=30000", timeout=35)
    data = resp.json()
    
    if data['count'] > 0:
        print(f"\n✓✓✓ SUCCESS! Received {data['count']} message(s) from CAN bus!")
        print("\nThis means POWERCELL is powered and transmitting!\n")
        for i, msg in enumerate(data['messages'], 1):
            data_hex = ' '.join([f"{b:02X}" for b in msg['data']])
            print(f"{i}. ID: 0x{msg['id']:08X} | Time: {msg['timestamp']:6d}ms | Data: {data_hex}")
        print("\n" + "="*70)
        print("POWERCELL IS ACTIVE!")
        print("The device can receive but POWERCELL isn't responding to our commands.")
        print("Possible reasons:")
        print("  - POWERCELL not configured to respond to J1939 polls")
        print("  - Wrong source address (we're using 0x63)")
        print("  - Wrong PGN (we're using 0xFF41)")
        print("="*70)
    else:
        print("\n✗ No CAN traffic detected in 30 seconds")
        print("\nThis means:")
        print("  1. POWERCELL is not powered, OR")
        print("  2. POWERCELL is not transmitting anything, OR")
        print("  3. CAN RX wiring issue (can send but not receive)")
        print("\nNext steps:")
        print("  [ ] Verify POWERCELL has power (check LED indicators)")
        print("  [ ] Check if POWERCELL transmits periodic messages")
        print("  [ ] Swap GPIO 19/20 (TX/RX might be reversed)")
        print("  [ ] Use CAN analyzer to verify POWERCELL is transmitting")
        
except Exception as e:
    print(f"\n✗ Error: {e}")

print("\n" + "="*70)
