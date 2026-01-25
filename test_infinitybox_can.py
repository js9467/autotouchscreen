#!/usr/bin/env python3
"""
Simple script to test communication with Infinitybox POWERCELL NGX over serial COM5.
This script allows you to send CAN messages and receive responses.
"""

import serial
import time
import sys

def connect_serial(port='COM5', baud=115200):
    """Connect to the serial port"""
    try:
        ser = serial.Serial(port, baud, timeout=2)
        time.sleep(2)  # Wait for connection to stabilize
        print(f"✓ Connected to {port} at {baud} baud")
        return ser
    except Exception as e:
        print(f"✗ Error connecting to {port}: {e}")
        return None

def send_command(ser, command):
    """Send a command and wait for response"""
    ser.write((command + '\n').encode())
    time.sleep(0.5)
    
    # Read all available data
    response = ""
    while ser.in_waiting > 0:
        response += ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
        time.sleep(0.1)
    
    return response

def poll_powercell(ser, address=1):
    """Poll a POWERCELL NGX at the specified address"""
    print(f"\n[TEST] Polling POWERCELL NGX at address {address}...")
    response = send_command(ser, f"canpoll {address}")
    print(response if response else "[No response]")
    return response

def monitor_can(ser, duration=10):
    """Monitor CAN bus for specified duration"""
    print(f"\n[TEST] Monitoring CAN bus for {duration} seconds...")
    response = send_command(ser, "canmon")
    print(response if response else "[No messages received]")
    return response

def check_can_status(ser):
    """Check CAN bus status"""
    print("\n[TEST] Checking CAN bus status...")
    response = send_command(ser, "canstatus")
    print(response if response else "[No response]")
    return response

def main():
    print("=" * 60)
    print(" Infinitybox POWERCELL NGX CAN Communication Test")
    print("=" * 60)
    
    # Connect to serial
    ser = connect_serial('COM5', 115200)
    if not ser:
        print("\n✗ Failed to connect. Make sure:")
        print("  1. Device is connected to COM5")
        print("  2. No other program is using COM5")
        print("  3. Device has correct firmware")
        sys.exit(1)
    
    try:
        # Clear any existing data
        ser.reset_input_buffer()
        ser.reset_output_buffer()
        
        # Test 1: Check if device responds
        print("\n[TEST 1] Testing serial communication...")
        response = send_command(ser, "help")
        if response:
            print("✓ Device is responding!")
            print(response[:200] + "..." if len(response) > 200 else response)
        else:
            print("✗ No response from device")
            print("  The device may not have the updated firmware with CAN commands")
            ser.close()
            sys.exit(1)
        
        # Test 2: Check CAN status
        check_can_status(ser)
        
        # Test 3: Monitor for any CAN messages
        monitor_can(ser, 5)
        
        # Test 4: Try polling POWERCELL address 1
        poll_powercell(ser, 1)
        
        # Interactive mode
        print("\n" + "=" * 60)
        print(" Interactive Mode - Enter commands (type 'quit' to exit)")
        print("=" * 60)
        print("\nAvailable commands:")
        print("  canstatus      - Check CAN bus status")
        print("  canmon         - Monitor CAN for 10 seconds")
        print("  canpoll <1-16> - Poll POWERCELL at address")
        print("  help           - Show all device commands")
        print("  quit           - Exit this script\n")
        
        while True:
            try:
                cmd = input("> ").strip()
                if cmd.lower() == 'quit':
                    break
                if cmd:
                    response = send_command(ser, cmd)
                    if response:
                        print(response)
                    else:
                        print("[No response]")
            except KeyboardInterrupt:
                print("\n\nExiting...")
                break
        
    finally:
        ser.close()
        print("\n✓ Serial connection closed")

if __name__ == "__main__":
    main()
