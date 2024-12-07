import serial
from enum import Enum

class MotorDirections(Enum):
    FORWARD = b"w"
    BACKWARD = b"s"
    LEFT = b"a"
    RIGHT = b"d"
    STOP = b"x"

class ArduinoUno:
    # Make a thread to read sensors
    # If any of the sensors are in a "we should stop" state, raise an exception
    # Be able to read sensor data

    def __init__(self, port='/dev/ttyACM0', baudrate=9600, timeout=1):
        self.ser = serial.Serial(port, baudrate, timeout=timeout)
        self.ser.reset_input_buffer()

    def send_direction(self, motor_direction: MotorDirections):
        self.ser.write(motor_direction.value)

    def close(self):
        self.ser.close()