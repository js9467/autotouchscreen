#!/usr/bin/env python3
"""
Waveshare ESP32-S3-Touch-LCD-7 CAN Diagnostic Test
Tests both pin configurations from official documentation
"""

import requests
import time

BASE_URL = "http://192.168.7.116"

def test_can_config(tx_pin, rx_pin, duration=5):
    """Test a specific TX/RX pin configuration"""
    print(f"\n{'='*70}")
    print(f"Testing: TX=GPIO{tx_pin}, RX=GPIO{rx_pin}")
    print(f"{'='*70}")
    
    # Reinitialize CAN with this configuration
    print(f"Reinitializing CAN bus...")
    try:
        response = requests.post(f"{BASE_URL}/api/can/reinit?tx_pin={tx_pin}&rx_pin={rx_pin}", timeout=5)
        time.sleep(2)  # Let driver start up
    except requests.exceptions.RequestException as e:
        print(f"  (Connection closed during reinit - this is normal)")
    
    # Check status
    print(f"Checking CAN status...")
    time.sleep(1)
    try:
        status = requests.get(f"{BASE_URL}/api/can/status", timeout=5).json()
        print(f"  Ready: {status.get('ready')}")
        print(f"  State: {status.get('state')} (1=RUNNING, 2=BUS_OFF)")
        print(f"  TX Errors: {status.get('tx_errors')}")
        print(f"  RX Errors: {status.get('rx_errors')}")
        print(f"  TX Queue: {status.get('tx_queue')}")
        print(f"  RX Queue: {status.get('rx_queue')}")
        
        rx_errors_before = status.get('rx_errors', 0)
    except Exception as e:
        print(f"  ✗ Failed to get status: {e}")
        return False
    
    # Listen for messages
    print(f"\nListening for {duration} seconds...")
    messages_received = 0
    start_time = time.time()
    
    while time.time() - start_time < duration:
        try:
            response = requests.get(f"{BASE_URL}/api/can/receive?timeout=1000", timeout=2)
            if response.status_code == 200:
                data = response.json()
                if data.get('success') and data.get('messages'):
                    for msg in data['messages']:
                        messages_received += 1
                        print(f"  ✓ Received: ID=0x{msg['id']:X}, Data={msg['data']}")
        except:
            pass
    
    # Final status check
    try:
        status = requests.get(f"{BASE_URL}/api/can/status", timeout=5).json()
        rx_errors_after = status.get('rx_errors', 0)
        print(f"\nFinal Status:")
        print(f"  Messages Received: {messages_received}")
        print(f"  RX Errors: {rx_errors_before} → {rx_errors_after} (delta: {rx_errors_after - rx_errors_before})")
        
        if messages_received > 0:
            print(f"\n  ✓✓✓ SUCCESS! This configuration WORKS!")
            return True
        elif rx_errors_after > rx_errors_before:
            print(f"\n  ⚠ RX errors increasing = receiving INVALID frames")
            print(f"     (Electrical signal present but not valid CAN)")
            return False
        else:
            print(f"\n  ✗ No activity detected on RX pin")
            return False
            
    except Exception as e:
        print(f"  ✗ Failed final status check: {e}")
        return False

def main():
    print("="*70)
    print("WAVESHARE ESP32-S3-Touch-LCD-7 CAN DIAGNOSTIC")
    print("="*70)
    print("\nAccording to Waveshare documentation:")
    print("  - CAN Transceiver: TJA1051")
    print("  - Demo code shows: TX=20, RX=19")
    print("  - Our testing showed: TX=19,RX=20 had RX errors (activity)")
    print("\nTesting both configurations...")
    
    configs = [
        (20, 19, "Waveshare official configuration"),
        (19, 20, "Swapped configuration (showed RX errors in testing)"),
    ]
    
    working_config = None
    
    for tx, rx, description in configs:
        print(f"\n\n{description}")
        if test_can_config(tx, rx, duration=5):
            working_config = (tx, rx)
            break
    
    print("\n" + "="*70)
    print("FINAL RESULTS")
    print("="*70)
    
    if working_config:
        print(f"✓ WORKING CONFIGURATION FOUND:")
        print(f"  TX Pin: GPIO{working_config[0]}")
        print(f"  RX Pin: GPIO{working_config[1]}")
        
        # Save to file
        with open("can_working_config.txt", "w") as f:
            f.write(f"TX_PIN={working_config[0]}\n")
            f.write(f"RX_PIN={working_config[1]}\n")
        print(f"\n  Configuration saved to: can_working_config.txt")
    else:
        print("✗ NO WORKING CONFIGURATION FOUND")
        print("\nPossible issues:")
        print("  1. CAN transceiver not powered (check 3.3V or 5V supply)")
        print("  2. CAN transceiver in wrong mode (check RS/STBY pins)")
        print("  3. POWERCELL devices not transmitting")
        print("  4. CAN H/L wiring issue at transceiver")
        print("  5. CAN transceiver damaged")
        print("\nNext steps:")
        print("  [ ] Verify TJA1051 has 3.3V power on pin 3 (VCC)")
        print("  [ ] Check TJA1051 pin 8 (S) is pulled to GND (silent mode off)")
        print("  [ ] Measure voltage on CAN H/L during POWERCELL communication")
        print("  [ ] Use oscilloscope to verify CAN differential signals")

if __name__ == "__main__":
    main()
