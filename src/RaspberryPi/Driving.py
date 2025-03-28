import threading

import numpy as np
import pandas as pd
import scipy
import time

from scipy.spatial.transform import Rotation as Rot
from collections import deque

from src.RaspberryPi.SharedMemory import SharedMemory
from src.RaspberryPi.States import MotorDirections
from src.RaspberryPi.InternalException import InvalidDirection
from src.Arduino.ArduinoUno import ArduinoUno

# assumptions to begin with -> all neigborhood points do not directly touch walls


class Driving:
    def __init__(self):
        self.t_accel = 0.28  # in seconds, calculated
        self.t_const = 1.15  # in seconds, calculated
        self.t_rotaccel = 0.28  # in seconds, calculated w/ 1 estimated value
        self.t_rotconst = 0.77  # in seconds, calculated w/ 1 estimated value
        self.driving_direction_memory = SharedMemory("driving_direction", 10, create=True)
        self.arduino_uno = ArduinoUno()

        self.local_driving_memory = SharedMemory(shem_name="local_driving", size=10, create=True)
        self.directions_memory = SharedMemory(shem_name="directions", size=10000, create=True)
        self.driving_thread_running = True
        self.driving_thread = threading.Thread(target=self.driving)
        self.driving_thread.start()

    def driving(self):
        while self.driving_thread_running:
            local_driving_direction = self.local_driving_memory.read_local_driving()
            destination_driving = self.directions_memory.read_string()

            print(f"local_driving_direction: {local_driving_direction}")
            print(f"destination_driving: {destination_driving}")

            if destination_driving != "":
                print("\n\nDESTINATION DRIVE TIME\n")
                while destination_driving != "" and self.directions_memory.read_string() != "":
                    print(f"\tNew: {self.directions_memory.read_string()}")
                    print(f"\tCurrent: {destination_driving}")
                    next_direction = destination_driving[0]
                    if len(destination_driving) > 1:
                        destination_driving = destination_driving[1:]
                    else:
                        destination_driving = ""

                    match next_direction:
                        case "d":
                            self.drive_one_unit(MotorDirections.RIGHT)
                        case "w":
                            self.drive_one_unit(MotorDirections.FORWARD)
                        case "a":
                            self.drive_one_unit(MotorDirections.LEFT)

                    self.directions_memory.write_string(destination_driving)


                self.directions_memory.write_string("")
            else:
                self.drive_one_unit(local_driving_direction)

            time.sleep(1)

    def close(self):
        self.arduino_uno.close()

    def __quanternion_to_euler(self, gyroscope_data):
        rotation_quan = Rot.from_quat(gyroscope_data,
                                      scalar_first=True)  # scalar-last order – (x, y, z, w) or scalar-first order – (w, x, y, z)
        rotation_euler = rotation_quan.as_euler('xyz', degrees=True)
        anglular_speed = (rotation_euler[3])
        return (anglular_speed)

    def __angularspeed_to_angle(self, angular_speed_df):
        Angle = scipy.integrate.simpson(angular_speed_df, x=None, dx=0.002)
        return Angle

    def __full_error(self, destination, currentpos):
        x_error = destination[0] - currentpos[0]
        y_error = destination[1] - currentpos[1]
        total_error = np.hypot(x_error, y_error)
        angle_error = np.arctan2(x_error, y_error) * 180 / 3.14159
        error = (x_error, y_error, total_error, angle_error)
        return error

    def __drive_one_unit(self, time_to_drive, direction):

        self.arduino_uno.send_direction(direction)
        while time.time() < time_to_drive:
            if self.arduino_uno.stop:
                break
            #print("DIRECTION")

        t_backward = time.time() + self.t_accel
        self.arduino_uno.send_direction(MotorDirections.STOP)
        while time.time() < t_backward:
            #print("STOP")
            continue
        time.sleep(2)

    def drive_one_unit(self, direction):
        match direction:
            case MotorDirections.FORWARD:
                time_to_drive = time.time() + self.t_accel + self.t_const
            case MotorDirections.LEFT:
                time_to_drive = time.time() + self.t_rotaccel + self.t_rotconst
            case MotorDirections.RIGHT:
                time_to_drive = time.time() + self.t_rotaccel + self.t_rotconst
            case MotorDirections.BACKWARD:
                time_to_drive = time.time() + self.t_accel + self.t_const
            case MotorDirections.STOP:
                time_to_drive = 0
            case _:
                raise InvalidDirection()

        self.__drive_one_unit(time_to_drive, direction)

    def get_motor_direction(self, driving_mode, gyroscope_data, destination, local_input):
        #destination = (0,2) #received from aleks every ???s, assuming y and x respectively
                            #Running on the assumption that the next point will be updated every ???s and not once the trainer reaches its first point

        #important Assumptions (need rotational assumtions changed):
        # Mspeed = 0.7 in m/s
        # Maccel = 2.5 in m/s^2
        # Mrotspeed = 30 in deg/s, confirmed
        # Mrotaccel = 90, This is estimated

        #currently for moving 1m forwards and turning 30 degrees at a time
        while driving_mode == 0:
            if local_input == 1:
                t_forward = time.time() + self.t_accel + self.t_const
                while time.time() < t_forward:
                    self.driving_direction_memory.write_enum(MotorDirections.FORWARD)

                t_backward = time.time() + self.t_accel
                while time.time() < t_backward:
                    self.driving_direction_memory.write_enum(MotorDirections.BACKWARD)
                local_input = 3 #stop after completion

            if local_input == 2:
                t_left = self.t_rotaccel + self.t_rotconst
                while time.time() < t_left:
                    self.driving_direction_memory.write_enum(MotorDirections.LEFT)

                t_right = self.t_rotaccel
                while time.time() < t_right:
                    self.driving_direction_memory.write_enum(MotorDirections.RIGHT)
                local_input = 3 #stop after completion

            if local_input == 3:
                t_stop = time.time() + 2
                while time.time() < t_stop:
                    self.driving_direction_memory.write_enum(MotorDirections.STOP)

            if local_input == 4:
                t_right = self.t_rotaccel + self.t_rotconst
                while time.time() < t_right:
                    self.driving_direction_memory.write_enum(MotorDirections.RIGHT)

                t_left = self.t_rotaccel
                while time.time() < t_left:
                    self.driving_direction_memory.write_enum(MotorDirections.LEFT)
                local_input = 3 #stop after completion

        while driving_mode == 1:

            df_size = 100
            gyroscope_df = deque(maxlen=df_size)
            previous_angle = 0
            angle = 0
            t_IMU = 0

            if time.time() > t_IMU:
                gyroscope_df.append(gyroscope_data)
                t_IMU = time.time() + 0.002 #grabs and c

            if len(gyroscope_df) == df_size:
                euler_df = self.__quanternion_to_euler(gyroscope_data)
                previous_angle = self.__angularspeed_to_angle(euler_df)
                angle = previous_angle + angle

            currentpos = (0,0) #since position is always wrt to the liar it should always be zero
            error = self.__full_error(destination, currentpos)

            if error < 0 and (error[3] - angle < -5):
                drive = 0
                self.driving_direction_memory.write_enum(MotorDirections.LEFT)
            elif error > 0 and (error[3] - angle > 5):
                drive = 0
                self.driving_direction_memory.write_enum(MotorDirections.RIGHT)
            else:
                drive = 1

            #assuming 2m stopping distance for linear motion
            #assuming next point is >2 m away, wont move if next point is <2 m way
            if drive == 1 and (error[1] > 0) and (error[2] >= 2):
                self.driving_direction_memory.write_enum(MotorDirections.FORWARD)
            elif drive == 1 and (error[1] < 0) and (error[2] >= 2):
                self.driving_direction_memory.write_enum(MotorDirections.BACKWARD)
            else:
                self.driving_direction_memory.write_enum(MotorDirections.STOP)

