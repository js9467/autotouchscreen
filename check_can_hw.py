import requests

DEVICE_IP = "192.168.7.116"

print("Quick CAN Hardware Check")
print("="*60)

# Reinit with default pins
resp = requests.post(f"http://{DEVICE_IP}/api/can/reinit", 
                     json={"tx_pin": 20, "rx_pin": 19}, timeout=5)
print(f"Reinit: {resp.json()}")

# Get status
resp = requests.get(f"http://{DEVICE_IP}/api/can/status", timeout=3)
status = resp.json()

print(f"\nCAN Status:")
print(f"  Ready: {status.get('ready')}")
print(f"  State: {status.get('state')} (1=RUNNING, 2=BUS_OFF)")
print(f"  TX Errors: {status.get('tx_errors')}")
print(f"  RX Errors: {status.get('rx_errors')}")
print(f"  TX Queue: {status.get('tx_queue')}")
print(f"  RX Queue: {status.get('rx_queue')}")

print(f"\nPLEASE VERIFY:")
print(f"  1. Which GPIO pins is your CAN transceiver connected to?")
print(f"  2. Is the CAN transceiver receiving 3.3V or 5V power?")
print(f"  3. What CAN transceiver chip are you using?")
print(f"  4. Can you see activity on CAN H/L with a multimeter/scope?")
