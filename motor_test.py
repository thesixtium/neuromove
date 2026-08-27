#!/usr/bin/env python3

import serial
import time

PORT = "/dev/ttyACM0"
BAUD = 19200

print(f"Connecting to Arduino on {PORT}...")
arduino = serial.Serial(PORT, BAUD, timeout=1)

time.sleep(2)

print("Connected!")
print()
print("Motor test:")
print("	w = FORWARD")
print("	a = LEFT")
print("	d = RIGHT")
print("	q = QUIT")
print()

while True:
	command = input("Command: ").strip().lower()
	if command == "w":
		arduino.write(b"w")
		arduino.flush()
		print("Sent FORWARD (w)")
		time.sleep(1)
		arduino.write(b"x")
		arduino.flush()
		
	elif command == "d":
		arduino.write(b"d")
		arduino.flush()
		print("Sent RIGHT (d)")
		time.sleep(1)
		arduino.write(b"x")
		arduino.flush()
		
	elif command == "a":
		arduino.write(b"a")
		arduino.flush()
		print("Sent LEFT (a)")
		time.sleep(1)
		arduino.write(b"x")
		arduino.flush()
		
	elif command == "q":
		arduino.write(b"x")
		arduino.flush()
		print("Stopping...")
		break

arduino.close()
