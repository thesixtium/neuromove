#!/usr/bin/env python3
import time
from src.RaspberryPi.ArduinoUno import ArduinoUno, MotorDirections
from src.RaspberryPi.InternalException import *
from src.RaspberryPi.Socket import Socket
from src.RaspberryPi.SharedMemory import SharedMemory


class States(Enum):
    START = 1
    SETUP = 2
    LOCAL = 3
    DESTINATION = 4
    RECOVERY = 5
    OFF = 6


def main():
    # Starting variables
    state = States.START
    next_state = States.START
    current_exception = None
    arduino_uno = None
    eye_tracking_memory = None
    occupancy_grid_memory = None
    p300_socket = None
    initialized = False

    while state != States.OFF:
        try:
            if current_exception is not None:
                state = States.RECOVERY
            else:
                state = next_state

            match state:
                case States.START:
                    print("Start")
                    if not initialized:
                        arduino_uno = ArduinoUno()
                        next_state = States.SETUP
                        eye_tracking_memory = SharedMemory(shem_name="eye_tracking", size=1, create=True)
                        occupancy_grid_memory = SharedMemory(shem_name="occupancy_grid", size=284622, create=True)
                        p300_socket = Socket(12347, 12348)
                        initialized = True


                case States.SETUP:
                    print("Setup")
                    next_state = States.LOCAL


                case States.LOCAL:
                    print("Local")
                    while True:
                        arduino_uno.send_direction(MotorDirections.FORWARD)
                        time.sleep(1)
                        arduino_uno.send_direction(MotorDirections.LEFT)
                        time.sleep(1)
                        arduino_uno.send_direction(MotorDirections.RIGHT)
                        time.sleep(1)
                        arduino_uno.send_direction(MotorDirections.BACKWARD)
                        time.sleep(1)


                case States.DESTINATION:
                    print("Destination")
                    raise NotImplementedYet("Destination State")


                case States.RECOVERY:
                    print(f"Recovery")

                    # If is an error that we threw
                    if isinstance(current_exception, InternalException):
                        print(current_exception.print())
                        if current_exception.is_permanent():
                            next_state = States.OFF
                        else:
                            next_state = States.LOCAL

                    # If is an error that we didn't throw
                    elif isinstance(current_exception, Exception):
                        print(current_exception.args)
                        next_state = States.OFF

                    # If not an error
                    else:
                        raise EnteredRecoveryModeWithoutException()

                    # Set exception back to none
                    current_exception = None


                case States.OFF:
                    raise EnteredOffState()


                case _:
                    print("Unknown state")
                    raise UnknownFSMState()


        except Exception as e:
            current_exception = e


    if initialized:
        arduino_uno.close()
        eye_tracking_memory.close()
        p300_socket.close()
        occupancy_grid_memory.close()


    if isinstance(current_exception, InternalException):
        exit(current_exception.get_exception_id())

if __name__ == '__main__':
    main()