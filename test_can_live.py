#!/usr/bin/env python3
"""CAN testing tool - Connect to Bronco Controls via serial and test CAN frames"""
import serial
import time
import sys

PORT = 'COM5'
BAUD = 115200

def send_command(ser, cmd, wait=1.0):
    """Send command and capture response"""
    print(f"\n>>> {cmd}")
    ser.write((cmd + "\n").encode())  # Just \n, not \r\n
    ser.flush()
    time.sleep(wait)
    
    response = b''
    for _ in range(30):  # More attempts
        if ser.in_waiting:
            response += ser.read(ser.in_waiting)
        time.sleep(0.1)  # Longer waits
    
    text = response.decode('utf-8', errors='ignore').strip()
    if text:
        print(text)
    else:
        print("<no response>")
    return text

def main():
    print(f"Opening {PORT} at {BAUD}...")
    try:
        ser = serial.Serial(PORT, BAUD, timeout=2)
    except Exception as e:
        print(f"Error: {e}")
        return 1
    
    # Reset device to see boot output
    print("Triggering device reset...")
    try:
        ser.setDTR(False)
        ser.setRTS(False)
        time.sleep(0.5)
        ser.setDTR(True)
        ser.setRTS(True)
        time.sleep(3)  # Wait longer for boot
    except:
        pass
    
    # Read boot output
    time.sleep(0.5)
    if ser.in_waiting:
        boot_data = ser.read(ser.in_waiting)
        print("\n=== BOOT OUTPUT ===")
        print(boot_data.decode('utf-8', errors='ignore'))
        print("===================\n")
    
    ser.reset_input_buffer()
    
    print("\n=== SYSTEM INFO ===")
    send_command(ser, 'help', 1.5)
    send_command(ser, 'status', 1.0)
    
    print("\n=== CAN STATUS ===")
    send_command(ser, 'canstatus', 1.0)
    
    print("\n=== SENDING TEST FRAME ===")
    # InfinityBox test frame - PGN 0xFF41 (65345)
    send_command(ser, 'cansend FF41 11 00 00 00 00 00 00 00', 0.5)
    
    print("\n=== MONITORING CAN TRAFFIC (10 seconds) ===")
    print("Watching for frames from InfinityBox...")
    send_command(ser, 'canmon', 10.0)
    
    ser.close()
    print("\nDone.")
    return 0

if __name__ == '__main__':
    sys.exit(main())
