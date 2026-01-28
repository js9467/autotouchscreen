import requests
import json
import time

DEVICE_IP = "192.168.7.116"
BASE_URL = f"http://{DEVICE_IP}"

print("="*70)
print("CAN PIN CONFIGURATION AUTO-TESTER")
print("="*70)
print("\nTesting different TX/RX pin combinations to find working config...")
print("Since POWERCELLs are already communicating, we should see traffic!\n")

# Pin combinations to try (ESP32-S3 common CAN pins)
pin_configs = [
    {"tx": 20, "rx": 19, "name": "Default (TX=20, RX=19)"},
    {"tx": 19, "rx": 20, "name": "Swapped (TX=19, RX=20)"},
    {"tx": 21, "rx": 22, "name": "Alt 1 (TX=21, RX=22)"},
    {"tx": 22, "rx": 21, "name": "Alt 1 Swapped (TX=22, RX=21)"},
    {"tx": 4, "rx": 5, "name": "Alt 2 (TX=4, RX=5)"},
    {"tx": 5, "rx": 4, "name": "Alt 2 Swapped (TX=5, RX=4)"},
]

for config in pin_configs:
    print("\n" + "="*70)
    print(f"Testing: {config['name']}")
    print("-" * 70)
    
    # Reinitialize CAN with new pins
    try:
        reinit_data = {"tx_pin": config["tx"], "rx_pin": config["rx"]}
        resp = requests.post(f"{BASE_URL}/api/can/reinit", json=reinit_data, timeout=5)
        result = resp.json()
        
        if not result.get("success"):
            print(f"✗ Failed to initialize with these pins")
            continue
            
        print(f"✓ CAN reinitialized with TX=GPIO{config['tx']}, RX=GPIO{config['rx']}")
        
        # Wait a moment for bus to stabilize
        time.sleep(0.5)
        
        # Check status
        resp = requests.get(f"{BASE_URL}/api/can/status", timeout=3)
        status = resp.json()
        print(f"  State: {status.get('state')} | TX Errors: {status.get('tx_errors')} | RX Errors: {status.get('rx_errors')}")
        
        if status.get('bus_off'):
            print(f"  ✗ Bus-off state detected - wrong pins")
            continue
        
        # Listen for traffic (should see POWERCELL/MASTERCELL communication)
        print(f"  Listening for 3 seconds...")
        resp = requests.get(f"{BASE_URL}/api/can/receive?timeout=3000", timeout=5)
        data = resp.json()
        
        if data['count'] > 0:
            print(f"\n  ✓✓✓ SUCCESS! Received {data['count']} message(s)!")
            print(f"\n  CORRECT PIN CONFIGURATION:")
            print(f"    TX = GPIO {config['tx']}")
            print(f"    RX = GPIO {config['rx']}")
            print(f"\n  Messages received:")
            for i, msg in enumerate(data['messages'][:5], 1):  # Show first 5
                data_hex = ' '.join([f"{b:02X}" for b in msg['data']])
                print(f"    {i}. ID: 0x{msg['id']:08X} | Data: {data_hex}")
            
            if data['count'] > 5:
                print(f"    ... and {data['count'] - 5} more messages")
            
            print("\n" + "="*70)
            print("FOUND WORKING CONFIGURATION!")
            print("="*70)
            
            # Save this config
            with open("can_pin_config.txt", "w") as f:
                f.write(f"TX_PIN={config['tx']}\n")
                f.write(f"RX_PIN={config['rx']}\n")
            
            print("\nConfiguration saved to can_pin_config.txt")
            print("Update your firmware with these pins:")
            print(f"  DEFAULT_TX_PIN = {config['tx']}")
            print(f"  DEFAULT_RX_PIN = {config['rx']}")
            
            exit(0)
        else:
            print(f"  ✗ No traffic detected")
            
    except Exception as e:
        print(f"  ✗ Error: {e}")
        continue

print("\n" + "="*70)
print("NO WORKING PIN CONFIGURATION FOUND")
print("="*70)
print("\nTroubleshooting:")
print("  1. Verify CAN transceiver is powered (3.3V or 5V)")
print("  2. Check CAN transceiver TX/RX pins are connected to ESP32")
print("  3. Verify CAN H/L from transceiver reach the bus")
print("  4. Check that POWERCELLs are actually transmitting")
print("  5. Use oscilloscope/logic analyzer on ESP32 GPIO pins")
print("  6. Verify CAN transceiver chip is working")
