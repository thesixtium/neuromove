# pipreqs src --ignore src/LiDAR
# tach mod
# tach sync
# tach show --web

#!/usr/bin/env python3
import numpy as np

from src.Arduino.ArduinoUno import ArduinoUno

from src.RaspberryPi.InternalException import *
from src.RaspberryPi.Socket import Socket
from src.RaspberryPi.SharedMemory import SharedMemory
from src.RaspberryPi.point_selection import occupancy_grid_to_points
from src.RaspberryPi.States import States, DestinationDrivingStates

from src.LiDAR.build.RunLiDAR import RunLiDAR

def main():
    # Starting variables
    state = States.START
    next_state = States.START
    initialized = False
    current_exception = None

    # Memories
    eye_tracking_memory = None
    occupancy_grid_memory = None
    point_selection_memory = None
    local_driving_memory = None
    imu_memory = None
    requested_next_state_memory = None
    destination_driving_state_memory = None

    # Connections
    arduino_uno = None
    lidar = None
    p300_socket = None

    while state != States.OFF:
        try:
            # Requested next state from the UI
            requested_next_state = requested_next_state_memory.read_requested_next_state()
            if requested_next_state:
                next_state = requested_next_state

            # Advance state
            if current_exception is not None:
                state = States.RECOVERY
            else:
                state = next_state

            # Use state
            match state:
                case States.START:
                    print("Start")
                    if not initialized:
                        arduino_uno = ArduinoUno()
                        lidar = RunLiDAR()

                        eye_tracking_memory = SharedMemory(shem_name="eye_tracking", size=1, create=True)
                        local_driving_memory = SharedMemory(shem_name="local_driving", size=1, create=True)
                        requested_next_state_memory = SharedMemory(shem_name="requested_next_state", size=1, create=True)
                        occupancy_grid_memory = SharedMemory(shem_name="occupancy_grid", size=284622, create=True)
                        imu_memory = SharedMemory(shem_name="imu", size=284622, create=True)
                        point_selection_memory = SharedMemory(shem_name="point_selection", size=1000, create=True)
                        destination_driving_state_memory = SharedMemory(shem_name="destination_driving_state", size=1, create=True)

                        p300_socket = Socket(12347, 12348)

                        initialized = True

                        next_state = States.SETUP



                case States.SETUP:
                    print("Setup")
                    next_state = States.LOCAL


                case States.LOCAL:
                    print("Local")
                    arduino_uno.send_direction(local_driving_memory.read_local_driving())


                case States.DESTINATION:
                    print("Destination")

                    destination_driving_state = destination_driving_state_memory.read_destination_driving_state()
                    match destination_driving_state:
                        case DestinationDrivingStates.IDLE:
                            pass
                        case DestinationDrivingStates.MAP_ROOM:
                            # Get point selections
                            occupancy_grid = np.array(occupancy_grid_memory.read_grid())
                            origin = (occupancy_grid.shape[0] // 2, occupancy_grid.shape[1] // 2)
                            selected_points = occupancy_grid_to_points(occupancy_grid, origin, plot_result=True)
                            point_selection_memory.write_np_array(selected_points)

                            destination_driving_state_memory.write_string("s")
                        case DestinationDrivingStates.SELECT_DESTINATION:
                            pass
                        case DestinationDrivingStates.TRANSLATE_TO_MOVEMENT:
                            destination_driving_state_memory.write_string("d")
                            raise NotImplementedYet("DestinationDrivingStates.TRANSLATE_TO_MOVEMENT")
                        case DestinationDrivingStates.DRIVE:
                            destination_driving_state_memory.write_string("i")
                            raise NotImplementedYet("DestinationDrivingStates.DRIVE")
                        case _:
                            raise UnknownDestinationDrivingState(destination_driving_state)


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
        point_selection_memory.close()
        local_driving_memory.close()
        requested_next_state_memory.close()
        destination_driving_state_memory.close()
        imu_memory.close()

    if isinstance(current_exception, InternalException):
        exit(current_exception.get_exception_id())

if __name__ == '__main__':
    main()