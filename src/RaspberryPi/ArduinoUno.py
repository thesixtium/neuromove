import serial
from enum import Enum
import threading
from src.RaspberryPi.InternalException import SensorDistanceAlert, CouldNotOpenPort

class MotorDirections(Enum):
    FORWARD = b"w"
    BACKWARD = b"s"
    LEFT = b"a"
    RIGHT = b"d"
    STOP = b"x"

class Sensors(Enum):
    ULTRASONIC_1 = 1
    ULTRASONIC_2 = 2
    ULTRASONIC_3 = 3
    ULTRASONIC_4 = 4
    ULTRASONIC_5 = 5
    ULTRASONIC_6 = 6
    FORCE_SENSOR = 7

class ArduinoUno:

    def __init__(self, port='/dev/ttyACM0', baudrate=9600, timeout=1, ultrasonic_minimum_distance=10):
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

    def send_direction(self, motor_direction: MotorDirections):
        self.ser.write(motor_direction.value)

    def close(self):
        self.serial_read_thread_running = False

    def update(self, sensor: Sensors, value: int):
        self.sensor_values[sensor.value] = value
        if (sensor.value == 7 and value == 1) or (sensor.value != 7 and value <= self.ultrasonic_minimum_distance):
            raise SensorDistanceAlert(sensor.name)

    def serial_read(self):
        while self.serial_read_thread_running:
            print(self.ser.read())

        self.ser.close()
