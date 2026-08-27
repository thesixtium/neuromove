import time

import serial
from enum import Enum
import threading
import pyduinocli
from src.RaspberryPi.States import MotorDirections
from src.RaspberryPi.SharedMemory import SharedMemory

class Sensors(Enum):
    ULTRASONIC_1 = 1
    ULTRASONIC_2 = 2
    ULTRASONIC_3 = 3
    ULTRASONIC_4 = 4
    ULTRASONIC_5 = 5
    ULTRASONIC_6 = 6
    FORCE_SENSOR = 7

class ArduinoUno:

    def __init__(self, port='/dev/ttyACM0', baudrate=19200, timeout=1, ultrasonic_minimum_distance=1):
        try:
            arduino = pyduinocli.Arduino("./src/Arduino/arduino-cli")
            brds = arduino.board.list()

            arduino.compile(fqbn="arduino:avr:uno", sketch="./src/Arduino/Arduino.ino")

            #print("\n\nA R D U I N O   S T U F F: ", end="\t")
            #print("UPLOADING")

            arduino.upload(fqbn="arduino:avr:uno", sketch="./src/Arduino/Arduino.ino", port=port)

            #print("\n\nA R D U I N O   S T U F F: ", end="\t")
            #print("DONE UPLOADING")
        except Exception as e:
            print(e)
            print(e.args)
        self.sensor_values = dict()
        self.ultrasonic_minimum_distance = ultrasonic_minimum_distance

        # Open serial port
        try:
            self.ser = serial.Serial(port, baudrate, timeout=timeout)
            self.ser.reset_input_buffer()
        except:
            print("Arduino Error 1")

        # Start serial reading thread
        self.serial_read_thread_running = True
        self.serial_read_thread = threading.Thread(target=self.serial_read)
        self.serial_read_thread.start()

        self.stop = False

    def send_direction(self, motor_direction: MotorDirections):
        if self.stop:
            self.ser.write(MotorDirections.STOP.value)
        else:
            self.ser.write(motor_direction.value)

    def close(self):
        self.serial_read_thread_running = False
        self.serial_writing_thread_running = False
        
    def serial_read(self):
        while self.serial_read_thread_running:
            read = self.ser.read()

            if read == b'S':
                continue

        self.ser.close()
