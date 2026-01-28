#!/usr/bin/env python3
"""Monitor CAN bus for InfinityBox traffic"""
import requests
import json
import time
from datetime import datetime

print("=" * 60)
print("  CAN BUS MONITOR - Listening for InfinityBox Traffic")
print("=" * 60)
print(f"Start time: {datetime.now().strftime('%H:%M:%S')}")
print("Press Ctrl+C to stop\n")

frame_count = 0
try:
    while True:
        try:
            # Listen for 1 second
            response = requests.get(
                "http://192.168.7.116/api/can/receive?timeout=1000",
                timeout=5
            )
            
            data = response.json()
            
            if data['count'] > 0:
                frame_count += data['count']
                print(f"[{datetime.now().strftime('%H:%M:%S')}] Received {data['count']} frame(s)")
                
                for msg in data['messages']:
                    data_hex = ' '.join([f"{b:02X}" for b in msg['data']])
                    # Try to parse J1939 format
                    can_id = int(msg['id'], 16)
                    priority = (can_id >> 26) & 0x7
                    data_page = (can_id >> 24) & 0x1
                    pdu_format = (can_id >> 16) & 0xFF
                    pdu_specific = (can_id >> 8) & 0xFF
                    source = can_id & 0xFF
                    
                    print(f"  ID: 0x{msg['id']:08s}  Priority: {priority}  PGN: 0x{pdu_format:02X}{pdu_specific:02X}")
                    print(f"    Source: 0x{source:02X}  DLC: {len(msg['data'])}  Data: {data_hex}")
            
        except requests.exceptions.ConnectionError:
            print("[ERROR] Cannot connect to device. Is it online?")
            time.sleep(1)
        except requests.exceptions.Timeout:
            print("[ERROR] Request timeout")
            time.sleep(1)
        except Exception as e:
            print(f"[ERROR] {e}")
            time.sleep(1)
            
except KeyboardInterrupt:
    print(f"\n\nMonitoring stopped.")
    print(f"Total frames received: {frame_count}")
    print(f"End time: {datetime.now().strftime('%H:%M:%S')}")
