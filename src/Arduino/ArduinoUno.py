import serial
from enum import Enum
import threading
from src.RaspberryPi.InternalException import SensorDistanceAlert, CouldNotOpenPort, ArduinoNotConnected
import pyduinocli
from src.RaspberryPi.States import MotorDirections

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
        print("\nFlashing board")
        arduino = pyduinocli.Arduino("./src/Arduino/arduino-cli")
        print(f"\tArduino: {arduino}")
        brds = arduino.board.list()
        print(f"\tBoards: {brds}")

        port = brds['result'][0]['port']['address']
        print(f"\tPort: {port}")
        fqbn = brds['result'][0]['matching_boards'][0]['fqbn']
        print(f"\tFQBN: {fqbn}")

        arduino.compile(fqbn=fqbn, sketch="./src/Arduino/Arduino.ino")
        arduino.upload(fqbn=fqbn, sketch="./src/Arduino/Arduino.inoArduino", port=port)

        #self.sensor_values = dict()
        #self.ultrasonic_minimum_distance = ultrasonic_minimum_distance

        # Open serial port
        #port='COM3'
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
            read = self.ser.read()
            print(f"Arduino Read: {read}")
            #if read != b'':
            #    sensor_type = read[0]
            #    sensor_number = read[1]
            #    value = read[3]

            #    print(f"Sensor {sensor_type} #{sensor_number}: {value}")

            #    if sensor_type == "S":
            #        match sensor_number:
            #            case "1":
            #                self.update(Sensors.ULTRASONIC_1, value)
            #            case "2":
            #                self.update(Sensors.ULTRASONIC_2, value)
            #            case "3":
            #                self.update(Sensors.ULTRASONIC_3, value)
            #            case "4":
            #                self.update(Sensors.ULTRASONIC_4, value)
            #            case "5":
            #                self.update(Sensors.ULTRASONIC_5, value)
            #            case "6":
            #                self.update(Sensors.ULTRASONIC_6, value)
            #    elif sensor_type == "F":
            #        self.update(Sensors.FORCE_SENSOR, value)

        self.ser.close()
