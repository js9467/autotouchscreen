import serial
import time

# Connect to the device
port = 'COM5'
baud = 115200

print(f"Connecting to {port} at {baud} baud...")
ser = serial.Serial(port, baud, timeout=2)
time.sleep(2)  # Wait for connection to stabilize

def send_command(cmd):
    print(f"\n>>> Sending: {cmd}")
    ser.write(f"{cmd}\n".encode())
    time.sleep(1)  # Wait for response
    
    # Read all available data
    response = ""
    while ser.in_waiting > 0:
        data = ser.read(ser.in_waiting)
        response += data.decode('utf-8', errors='ignore')
        time.sleep(0.1)
    
    if response:
        print(f"<<< Response:\n{response}")
    else:
        print("<<< No response")
    return response

# Clear any existing data
ser.reset_input_buffer()
time.sleep(0.5)

print("\n" + "="*60)
print("Testing CAN Communication with Infinitybox POWERCELL NGX")
print("="*60)

# Test 1: Check CAN status
send_command("canstatus")

# Test 2: Poll POWERCELL NGX at address 1
send_command("canpoll 1")

# Test 3: Monitor CAN traffic (will run for 10 seconds)
print("\n>>> Starting CAN monitor (10 seconds)...")
ser.write(b"canmon\n")
start_time = time.time()
while time.time() - start_time < 11:  # Monitor for 11 seconds to see all output
    if ser.in_waiting > 0:
        data = ser.read(ser.in_waiting)
        print(data.decode('utf-8', errors='ignore'), end='')
    time.sleep(0.1)

# Test 4: Request config from address 1
send_command("canconfig 1")

# Test 5: Send custom message
send_command("cansend FF41 11 00 00 00 00 00 00 00")

print("\n" + "="*60)
print("Test complete!")
print("="*60)

ser.close()
