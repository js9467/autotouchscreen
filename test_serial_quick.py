import serial
import time

ser = serial.Serial('COM5', 115200, timeout=1)
time.sleep(1)

print("Sending: help")
ser.write(b'help\n')
ser.flush()

time.sleep(1)

print("\nResponse:")
while ser.in_waiting:
    data = ser.read(ser.in_waiting)
    print(data.decode('utf-8', errors='ignore'))
    time.sleep(0.1)

print("\nSending: canstatus")
ser.write(b'canstatus\n')
ser.flush()
time.sleep(1)

print("\nResponse:")
while ser.in_waiting:
    data = ser.read(ser.in_waiting)
    print(data.decode('utf-8', errors='ignore'))

ser.close()
