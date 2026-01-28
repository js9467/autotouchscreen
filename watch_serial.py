import serial
import time

port = 'COM5'
baud = 115200

print("Connecting to device...")
print("If the device reboots, you'll see boot messages.")
print("Press Ctrl+C to stop\n")
print("="*60)

try:
    ser = serial.Serial(port, baud, timeout=0.1)
    
    print("Watching for ANY serial output...")
    print("Try pressing the RESET button on the device now.")
    print("="*60 + "\n")
    
    last_time = time.time()
    byte_count = 0
    
    while True:
        if ser.in_waiting > 0:
            data = ser.read(ser.in_waiting)
            byte_count += len(data)
            print(data.decode('utf-8', errors='replace'), end='', flush=True)
            last_time = time.time()
        else:
            # Show we're still listening every 5 seconds
            if time.time() - last_time > 5 and byte_count == 0:
                print(f"[{time.strftime('%H:%M:%S')}] Still listening... (no data received yet)", flush=True)
                last_time = time.time()
        time.sleep(0.01)
        
except KeyboardInterrupt:
    print("\n\nStopped by user.")
    if byte_count > 0:
        print(f"Total bytes received: {byte_count}")
    else:
        print("No data was received from the device.")
except Exception as e:
    print(f"\nError: {e}")
finally:
    try:
        ser.close()
    except:
        pass
