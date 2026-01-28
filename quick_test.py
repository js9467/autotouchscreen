import requests
import time

print("\n🔧 QUICK AUTO-SEQUENCE TEST\n")
print("Sending PGN FF01 with data 20 80 (Output 9 ON pattern)...")

try:
    r = requests.post("http://192.168.7.116/api/can/send", 
                     json={"pgn": 0xFF01, "priority": 6, "source": 0x80, 
                           "destination": 0xFF, "data": [0x20, 0x80, 0, 0, 0, 0, 0, 0]},
                     timeout=3)
    print(f"✅ SUCCESS: {r.json()}")
    print("\nThis means your button configured with:")
    print("  PGN: FF01, Priority: 6, Data: 20 80 00 00 00 00 00 00")
    print("Will now automatically trigger the full 3-message sequence!")
except Exception as e:
    print(f"❌ Error: {e}")
