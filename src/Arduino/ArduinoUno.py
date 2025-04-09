import serial
from enum import Enum

from src.RaspberryPi.InternalException import CouldNotOpenPort
import pyduinocli
import time

class MotorDirections(Enum):
    FORWARD = b"w"
    BACKWARD = b"s"
    LEFT = b"a"
    RIGHT = b"d"
    STOP = b"x"

class ArduinoUno:
    def __init__(self, port='/dev/ttyACM0', baudrate=19200, timeout=1):
        self.t_accel = 0.28  # in seconds, calculated
        self.t_const = 1.15  # in seconds, calculated
        self.t_rotaccel = 0.28  # in seconds, calculated w/ 1 estimated value
        self.t_rotconst = 0.77  # in seconds, calculated w/ 1 estimated value

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

    def drive_one_unit(self, direction):
        match direction:
            case MotorDirections.FORWARD:
                time_to_drive = self.t_accel + self.t_const
            case MotorDirections.LEFT:
                time_to_drive = self.t_rotaccel + self.t_rotconst
            case MotorDirections.RIGHT:
                time_to_drive = self.t_rotaccel + self.t_rotconst
            case MotorDirections.BACKWARD:
                time_to_drive = self.t_accel + self.t_const
            case _:
                direction = MotorDirections.STOP
                time_to_drive = 0

        self.ser.write(direction.value)
        time.sleep(time_to_drive)

    def drive_path(self, directions):
        for direction in directions:
            self.drive_one_unit(direction)
