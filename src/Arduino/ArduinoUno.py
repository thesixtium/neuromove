import serial
from enum import Enum
from src.RaspberryPi.InternalException import CouldNotOpenPort
import pyduinocli

class MotorDirections(Enum):
    FORWARD = b"w"
    BACKWARD = b"s"
    LEFT = b"a"
    RIGHT = b"d"
    STOP = b"x"

class ArduinoUno:
    def __init__(self, port='/dev/ttyACM0', baudrate=19200, timeout=1):
        # Upload arduino code
        try:
            arduino = pyduinocli.Arduino("./src/Arduino/arduino-cli")
            arduino.compile(fqbn="arduino:avr:uno", sketch="./src/Arduino/Arduino.ino")
            arduino.upload(fqbn="arduino:avr:uno", sketch="./src/Arduino/Arduino.ino", port=port)
        except Exception as e:
            print(e)
            print(e.args)

        # Open serial port
        try:
            self.ser = serial.Serial(port, baudrate, timeout=timeout)
            self.ser.reset_input_buffer()
        except:
            raise CouldNotOpenPort(port)

    def send_direction(self, motor_direction: MotorDirections):
        self.ser.write(motor_direction.value)
