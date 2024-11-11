#!/usr/bin/env python3
import serial
import time

def main():
    ser = serial.Serial('/dev/ttyACM0', 9600, timeout=1)
    ser.reset_input_buffer()

    while True:
        ser.write(b"w")
        time.sleep(1)
        ser.write(b"a")
        time.sleep(1)
        ser.write(b"s")
        time.sleep(1)
        ser.write(b"d")
        time.sleep(1)

if __name__ == '__main__':
    main()