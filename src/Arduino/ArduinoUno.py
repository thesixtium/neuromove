import time

import serial
from enum import Enum
import threading
from src.RaspberryPi.InternalException import SensorDistanceAlert, CouldNotOpenPort, ArduinoNotConnected
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

            print("\n\nA R D U I N O   S T U F F: ", end="\t")
            print("UPLOADING")

            arduino.upload(fqbn="arduino:avr:uno", sketch="./src/Arduino/Arduino.ino", port=port)

            print("\n\nA R D U I N O   S T U F F: ", end="\t")
            print("DONE UPLOADING")
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
            raise CouldNotOpenPort(port)

        # Start serial reading thread
        self.serial_read_thread_running = True
        self.serial_read_thread = threading.Thread(target=self.serial_read)
        self.serial_read_thread.start()

        self.stop = False

    def send_direction(self, motor_direction: MotorDirections):
        if self.stop:
            print("'Mergency Stop")
            self.ser.write(MotorDirections.STOP.value)
        else:
            print(f"Drive {motor_direction.value}")
            self.ser.write(motor_direction.value)

    def close(self):
        self.serial_read_thread_running = False
        self.serial_writing_thread_running = False

    def update(self, sensor: Sensors, value: int):
        self.sensor_values[sensor.value] = value

        stop_value = False
        for key in self.sensor_values:
            if (key == 7 and self.sensor_values[key] == 1) or (
                    key != 7 and self.sensor_values[key] <= self.ultrasonic_minimum_distance):
                self.stop = True

        self.stop = stop_value


    def serial_read(self):
        while self.serial_read_thread_running:
            read = self.ser.read()

            if read == b'S':
                sensor_type = read.decode()
                sensor_number = self.ser.read().decode()
                value = ""
                while True:
                    read = self.ser.read().decode()
                    if read == "\r" or read == "\n":
                        break
                    value += read
                value = float(value)

                if sensor_type == "S":
                    match sensor_number:
                        case "1":
                            self.update(Sensors.ULTRASONIC_1, value)
                        case "2":
                            self.update(Sensors.ULTRASONIC_2, value)
                        case "3":
                            self.update(Sensors.ULTRASONIC_3, value)
                        case "4":
                            self.update(Sensors.ULTRASONIC_4, value)
                        case "5":
                            self.update(Sensors.ULTRASONIC_5, value)
                        case "6":
                            self.update(Sensors.ULTRASONIC_6, value)
                elif sensor_type == "F":
                    self.update(Sensors.FORCE_SENSOR, value)

        self.ser.close()
