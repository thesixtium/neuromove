# Goal:
# - Test Arduino Control
# - Test ultrasonics and FSRs

from src.RaspberryPi.ArduinoUno import ArduinoUno
from src.RaspberryPi.ArduinoUno import MotorDirections

def main():
    arduino_uno = ArduinoUno()

    running = True

    while running:
        direction = MotorDirections.STOP

        command = input("Command: ")
        match command:
            case "w":
                direction = MotorDirections.FORWARD
            case "a":
                direction = MotorDirections.LEFT
            case "s":
                direction = MotorDirections.BACKWARD
            case "d":
                direction = MotorDirections.RIGHT
            case "q":
                running = False

        arduino_uno.send_direction(direction)
    arduino_uno.close()

if __name__ == '__main__':
    main()